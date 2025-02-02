/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "audio_element.h"

#include "rom/queue.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_mem.h"
#include "audio_common.h"
#include "audio_mutex.h"
#include "ringbuf.h"
#include "audio_error.h"

static const char *TAG = "AUDIO_PIPELINE";

typedef struct ringbuf_item {
    STAILQ_ENTRY(ringbuf_item)  next;
    ringbuf_handle_t            rb;
    int                         rb_size;
} ringbuf_item_t;

typedef STAILQ_HEAD(ringbuf_list, ringbuf_item) ringbuf_list_t;

typedef struct audio_element_item {
    STAILQ_ENTRY(audio_element_item) next;
    audio_element_handle_t           el;
    int                              index;
    int                              rb_sz;  // 0:by default; >0: ringbuffer size.
    bool                             linked;
    audio_element_status_t           el_state;
} audio_element_item_t;

typedef STAILQ_HEAD(audio_element_list, audio_element_item) audio_element_list_t;

struct audio_pipeline {
    audio_element_list_t        el_list;
    ringbuf_list_t              rb_list;
    audio_element_state_t       state;
    xSemaphoreHandle            lock;
    int                         rb_size;
    bool                        linked;
    audio_event_iface_handle_t  listener;
};

static audio_element_item_t *audio_pipeline_get_el_by_tag(audio_pipeline_handle_t pipeline, const char *tag)
{
    audio_element_item_t *item;
    STAILQ_FOREACH(item, &pipeline->el_list, next) {
        char *el_tag = audio_element_get_tag(item->el);
        if (el_tag && strcasecmp(el_tag, tag) == 0) {
            return item;
        }
    }
    return NULL;
}

static esp_err_t audio_pipeline_change_state(audio_pipeline_handle_t pipeline, audio_element_state_t new_state)
{
    // mutex_lock(pipeline->lock);
    pipeline->state = new_state;
    // mutex_unlock(pipeline->lock);
    return ESP_OK;
}

static void audio_pipeline_register_element(audio_pipeline_handle_t pipeline, audio_element_handle_t el)
{
    audio_element_item_t *el_item = audio_calloc(1, sizeof(audio_element_item_t));
    AUDIO_MEM_CHECK(TAG, el_item, return);
    el_item->el = el;
    el_item->linked = true;
    STAILQ_INSERT_TAIL(&pipeline->el_list, el_item, next);
}

static void audio_pipeline_unregister_element(audio_pipeline_handle_t pipeline, audio_element_handle_t el)
{
    audio_element_item_t *el_item, *tmp;
    STAILQ_FOREACH_SAFE(el_item, &pipeline->el_list, next, tmp) {
        if (el_item->el == el) {
            STAILQ_REMOVE(&pipeline->el_list, el_item, audio_element_item, next);
            audio_free(el_item);
        }
    }
}

static void add_rb_to_audio_pipeline(audio_pipeline_handle_t pipeline, ringbuf_handle_t rb)
{
    ringbuf_item_t *rb_item = (ringbuf_item_t *)audio_calloc(1, sizeof(ringbuf_item_t));
    AUDIO_MEM_CHECK(TAG, rb_item, return);
    rb_item->rb = rb;
    rb_item->rb_size = rb_size_get(rb);
    STAILQ_INSERT_TAIL(&pipeline->rb_list, rb_item, next);
}

esp_err_t audio_pipeline_set_listener(audio_pipeline_handle_t pipeline, audio_event_iface_handle_t listener)
{
    audio_element_item_t *el_item;
    if (pipeline->listener) {
        audio_pipeline_remove_listener(pipeline);
    }
    STAILQ_FOREACH(el_item, &pipeline->el_list, next) {
        if (audio_element_msg_set_listener(el_item->el, listener) != ESP_OK) {
            ESP_LOGE(TAG, "Error register event with: %s", (char *)audio_element_get_tag(el_item->el));
            return ESP_FAIL;
        }
    }
    pipeline->listener = listener;
    return ESP_OK;
}

esp_err_t audio_pipeline_remove_listener(audio_pipeline_handle_t pipeline)
{
    audio_element_item_t *el_item;
    if (pipeline->listener == NULL) {
        ESP_LOGW(TAG, "There are no listener registered");
        return ESP_FAIL;
    }
    STAILQ_FOREACH(el_item, &pipeline->el_list, next) {
        if (audio_element_msg_remove_listener(el_item->el, pipeline->listener) != ESP_OK) {
            ESP_LOGE(TAG, "Error unregister event with: %s", audio_element_get_tag(el_item->el));
            return ESP_FAIL;
        }
    }
    pipeline->listener = NULL;
    return ESP_OK;
}

audio_pipeline_handle_t audio_pipeline_init(audio_pipeline_cfg_t *config)
{
    audio_pipeline_handle_t pipeline;
    bool _success =
        (
            (pipeline       = audio_calloc(1, sizeof(struct audio_pipeline)))   &&
            (pipeline->lock = mutex_create())
        );

    AUDIO_MEM_CHECK(TAG, _success, return NULL);
    STAILQ_INIT(&pipeline->el_list);
    STAILQ_INIT(&pipeline->rb_list);

    pipeline->state = AEL_STATE_INIT;
    pipeline->rb_size = config->rb_size;

    return pipeline;
}

esp_err_t audio_pipeline_deinit(audio_pipeline_handle_t pipeline)
{
    audio_pipeline_terminate(pipeline);
    audio_pipeline_unlink(pipeline);
    mutex_destroy(pipeline->lock);
    audio_free(pipeline);
    return ESP_OK;
}

esp_err_t audio_pipeline_register(audio_pipeline_handle_t pipeline, audio_element_handle_t el, const char *name)
{
    audio_pipeline_unregister(pipeline, el);
    audio_element_set_tag(el, name);
    audio_element_item_t *el_item = audio_calloc(1, sizeof(audio_element_item_t));

    AUDIO_MEM_CHECK(TAG, el_item, return ESP_ERR_NO_MEM);
    el_item->el = el;
    el_item->linked = false;
    STAILQ_INSERT_TAIL(&pipeline->el_list, el_item, next);
    return ESP_OK;
}

esp_err_t audio_pipeline_unregister(audio_pipeline_handle_t pipeline, audio_element_handle_t el)
{
    audio_element_set_tag(el, NULL);

    audio_element_item_t *el_item, *tmp;
    STAILQ_FOREACH_SAFE(el_item, &pipeline->el_list, next, tmp) {
        if (el_item->el == el) {
            STAILQ_REMOVE(&pipeline->el_list, el_item, audio_element_item, next);
            audio_free(el_item);
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

esp_err_t audio_pipeline_resume(audio_pipeline_handle_t pipeline)
{
    audio_element_item_t *el_item;
    bool wait_first_el = true;
    esp_err_t ret = ESP_OK;
    STAILQ_FOREACH(el_item, &pipeline->el_list, next) {
        ESP_LOGD(TAG, "audio_pipeline_resume,linked:%d,state:%d,[%p]", el_item->linked, audio_element_get_state(el_item->el), el_item->el);
        if (false == el_item->linked) {
            continue;
        }
        if (wait_first_el) {
            ret |= audio_element_resume(el_item->el, 0, portMAX_DELAY);
            wait_first_el = false;
        } else {
            ret |= audio_element_resume(el_item->el, 0, 0);
        }
    }
    return ret;
}

esp_err_t audio_pipeline_pause(audio_pipeline_handle_t pipeline)
{
    audio_element_item_t *el_item;
    STAILQ_FOREACH(el_item, &pipeline->el_list, next) {
        ESP_LOGD(TAG, "audio_pipeline_pause [%s]  %p", audio_element_get_tag(el_item->el), el_item->el);
        audio_element_pause(el_item->el);
    }

    return ESP_OK;
}

esp_err_t audio_pipeline_state(audio_pipeline_handle_t pipeline)
{
    return pipeline->state;
}

esp_err_t audio_pipeline_run(audio_pipeline_handle_t pipeline)
{
    audio_element_item_t *el_item;
    if (pipeline->state != AEL_STATE_INIT) {
        ESP_LOGW(TAG, "Pipeline already started");
        return ESP_OK;
    }
    STAILQ_FOREACH(el_item, &pipeline->el_list, next) {
        ESP_LOGD(TAG, "start el, linked:%d,state:%d,[%p]", el_item->linked,  audio_element_get_state(el_item->el), el_item->el);
        if (el_item->linked
            && ((AEL_STATE_INIT == audio_element_get_state(el_item->el))
                || (AEL_STATE_STOPPED == audio_element_get_state(el_item->el))
                || (AEL_STATE_FINISHED == audio_element_get_state(el_item->el))
                || (AEL_STATE_ERROR == audio_element_get_state(el_item->el)))) {
            audio_element_run(el_item->el);
        }
    }
    AUDIO_MEM_SHOW(TAG);

    if (ESP_FAIL == audio_pipeline_resume(pipeline)) {
        ESP_LOGE(TAG, "audio_pipeline_resume failed");
        audio_pipeline_change_state(pipeline, AEL_STATE_ERROR);
        audio_pipeline_terminate(pipeline);
        return ESP_FAIL;
    } else {
        audio_pipeline_change_state(pipeline, AEL_STATE_RUNNING);
    }

    ESP_LOGI(TAG, "Pipeline started");
    return ESP_OK;
}

esp_err_t audio_pipeline_terminate(audio_pipeline_handle_t pipeline)
{
    audio_element_item_t *el_item;
    ESP_LOGD(TAG, "Destroy audio_pipeline elements");
    audio_pipeline_stop(pipeline);
    audio_pipeline_wait_for_stop(pipeline);
    STAILQ_FOREACH(el_item, &pipeline->el_list, next) {
        if (el_item->linked) {
            audio_element_terminate(el_item->el);
        }
    }
    return ESP_OK;
}

esp_err_t audio_pipeline_stop(audio_pipeline_handle_t pipeline)
{
    audio_element_item_t *el_item;
    ESP_LOGD(TAG, "audio_element_stop");
    STAILQ_FOREACH(el_item, &pipeline->el_list, next) {
        if (el_item->linked) {
            audio_element_stop(el_item->el);
        }
    }
    return ESP_OK;
}

esp_err_t audio_pipeline_wait_for_stop(audio_pipeline_handle_t pipeline)
{
    audio_element_item_t *el_item;
    ESP_LOGD(TAG, "audio_pipeline_wait_for_stop");
    if (pipeline->state != AEL_STATE_RUNNING) {
        return ESP_FAIL;
    }
    STAILQ_FOREACH(el_item, &pipeline->el_list, next) {
        if (el_item->linked) {
            audio_element_wait_for_stop(el_item->el);
            audio_element_reset_input_ringbuf(el_item->el);
            audio_element_reset_output_ringbuf(el_item->el);
        }
    }
    audio_pipeline_change_state(pipeline, AEL_STATE_INIT);
    return ESP_OK;
}

esp_err_t audio_pipeline_link(audio_pipeline_handle_t pipeline, const char *link_tag[], int link_num)
{
    int i, idx = 0;
    bool first = false, last = false;
    ringbuf_handle_t rb = NULL;
    ringbuf_item_t *rb_item;

    if (pipeline->linked) {
        audio_pipeline_unlink(pipeline);
    }
    for (i = 0; i < link_num; i++) {
        audio_element_item_t *item = audio_pipeline_get_el_by_tag(pipeline, link_tag[i]);
        if (item == NULL) {
            ESP_LOGE(TAG, "There are 1 link_tag invalid: %s", link_tag[i]);
            return ESP_FAIL;
        }
        item->index = idx ++;
        item->linked = true;
        audio_element_handle_t el = item->el;

        /* Create and Link ringubffer */
        first = (i == 0);
        last = (i == link_num - 1);
        if (last) {
            audio_element_set_input_ringbuf(el, rb);
        } else {
            if (!first) {
                audio_element_set_input_ringbuf(el, rb);
            }
            bool _success = (
                                (rb_item = audio_calloc(1, sizeof(ringbuf_item_t))) &&
                                (rb = rb_create(audio_element_get_output_ringbuf_size(el), 1))
                            );

            AUDIO_MEM_CHECK(TAG, _success, {
                free(rb_item);
                return ESP_ERR_NO_MEM;
            });

            rb_item->rb = rb;
            rb_item->rb_size = pipeline->rb_size;
            STAILQ_INSERT_TAIL(&pipeline->rb_list, rb_item, next);
            audio_element_set_output_ringbuf(el, rb);
        }

    }
    pipeline->linked = true;
    return ESP_OK;

}

esp_err_t audio_pipeline_unlink(audio_pipeline_handle_t pipeline)
{
    audio_element_item_t *el_item;
    ringbuf_item_t *rb_item, *tmp;

    if (!pipeline->linked) {
        return ESP_OK;
    }
    audio_pipeline_remove_listener(pipeline);
    STAILQ_FOREACH(el_item, &pipeline->el_list, next) {
        if (el_item->linked) {
            el_item->linked = false;
            audio_element_set_output_ringbuf(el_item->el, NULL);
            audio_element_set_input_ringbuf(el_item->el, NULL);
            ESP_LOGD(TAG, "audio_pipeline_unlink,%p, %s", el_item->el, audio_element_get_tag(el_item->el));
        }
    }
    STAILQ_FOREACH_SAFE(rb_item, &pipeline->rb_list, next, tmp) {
        STAILQ_REMOVE(&pipeline->rb_list, rb_item, ringbuf_item, next);
        rb_destroy(rb_item->rb);
        audio_free(rb_item);
        ESP_LOGD(TAG, "audio_pipeline_unlink, RB, %p,", rb_item->rb);
    }
    ESP_LOGI(TAG, "audio_pipeline_unlinked");
    STAILQ_INIT(&pipeline->rb_list);
    pipeline->linked = false;
    return ESP_OK;
}

esp_err_t audio_pipeline_register_more(audio_pipeline_handle_t pipeline, audio_element_handle_t element_1, ...)
{
    va_list args;
    va_start(args, element_1);
    while (element_1) {
        audio_pipeline_register_element(pipeline, element_1);
        element_1 = va_arg(args, audio_element_handle_t);
    }
    va_end(args);
    return ESP_OK;
}

esp_err_t audio_pipeline_unregister_more(audio_pipeline_handle_t pipeline, audio_element_handle_t element_1, ...)
{
    va_list args;
    va_start(args, element_1);
    while (element_1) {
        audio_pipeline_unregister_element(pipeline, element_1);
        element_1 = va_arg(args, audio_element_handle_t);
    }
    va_end(args);
    return ESP_OK;
}

esp_err_t audio_pipeline_link_more(audio_pipeline_handle_t pipeline, audio_element_handle_t element_1, ...)
{
    va_list args;
    int idx = 0;
    bool first = false;
    ringbuf_handle_t rb = NULL;
    if (pipeline->linked) {
        audio_pipeline_unlink(pipeline);
    }
    va_start(args, element_1);
    while (element_1) {
        audio_element_handle_t el = element_1;
        audio_element_item_t *el_item = audio_calloc(1, sizeof(audio_element_item_t));
        AUDIO_MEM_CHECK(TAG, el_item, return ESP_ERR_NO_MEM);
        el_item->el = el;
        el_item->linked = true;
        STAILQ_INSERT_TAIL(&pipeline->el_list, el_item, next);
        idx ++;
        first = (idx == 1);
        element_1 = va_arg(args, audio_element_handle_t);
        if (NULL == element_1) {
            audio_element_set_input_ringbuf(el, rb);
        } else {
            if (!first) {
                audio_element_set_input_ringbuf(el, rb);
            }
            rb = rb_create(audio_element_get_output_ringbuf_size(el), 1);
            AUDIO_MEM_CHECK(TAG, rb, return ESP_ERR_NO_MEM);
            add_rb_to_audio_pipeline(pipeline, rb);
            audio_element_set_output_ringbuf(el, rb);
        }
        ESP_LOGD(TAG, "element is %p,rb:%p", el, rb);
    }
    pipeline->linked = true;
    va_end(args);
    return ESP_OK;
}

esp_err_t audio_pipeline_link_insert(audio_pipeline_handle_t pipeline, bool first, audio_element_handle_t prev, ringbuf_handle_t conect_rb, audio_element_handle_t next)
{
    if (first) {
        audio_pipeline_register_element(pipeline, prev);
    }
    ESP_LOGD(TAG, "element is prev:%p,rb:%p,next:%p", prev, conect_rb, next);
    audio_pipeline_register_element(pipeline, next);
    audio_element_set_output_ringbuf(prev, conect_rb);
    audio_element_set_input_ringbuf(next, conect_rb);
    add_rb_to_audio_pipeline(pipeline, conect_rb);
    pipeline->linked = true;
    return ESP_OK;
}

esp_err_t audio_pipeline_listen_more(audio_pipeline_handle_t pipeline, audio_element_handle_t element_1, ...)
{
    va_list args;
    va_start(args, element_1);
    while (element_1) {
        audio_element_handle_t el = element_1;
        element_1 = va_arg(args, audio_element_handle_t);
        QueueHandle_t que = audio_element_get_event_queue(el);
        audio_event_iface_msg_t dummy = {0};
        while (1) {
            if (xQueueReceive(que, &dummy, 0) == pdTRUE) {
                ESP_LOGD(TAG, "Listen_more el:%p,que :%p,OK", el, que);
            } else {
                ESP_LOGD(TAG, "Listen_more el:%p,que :%p,FAIL", el, que);
                break;
            }
        }
    }
    va_end(args);
    return ESP_OK;
}

esp_err_t audio_pipeline_check_items_state(audio_pipeline_handle_t pipeline, audio_element_handle_t el, audio_element_status_t status)
{
    audio_element_item_t *item;
    int el_cnt = 0;
    int el_sta_cnt = 0;
    audio_element_item_t *it = audio_pipeline_get_el_by_tag(pipeline, audio_element_get_tag(el));
    it->el_state =  status;
    STAILQ_FOREACH(item, &pipeline->el_list, next) {
        if (false == item->linked) {
            continue;
        }
        el_cnt ++;
        ESP_LOGD(TAG, "pipeline check state,pipeline:%p,el:%p, state:%d, status:%d", pipeline, item->el, item->el_state, status);
        if ((AEL_STATUS_NONE != item->el_state) && (status == item->el_state)) {
            el_sta_cnt++;
        }
    }
    if (el_cnt && (el_sta_cnt == el_cnt)) {
        return ESP_OK;
    } else {
        return ESP_FAIL;
    }
}

esp_err_t audio_pipeline_reset_items_state(audio_pipeline_handle_t pipeline)
{
    audio_element_item_t *el_item;
    ESP_LOGD(TAG, "audio_pipeline_reset_items_state");
    STAILQ_FOREACH(el_item, &pipeline->el_list, next) {
        if (el_item->linked) {
            el_item->el_state = AEL_STATUS_NONE;
        }
    }
    return ESP_OK;
}

esp_err_t audio_pipeline_reset_ringbuffer(audio_pipeline_handle_t pipeline)
{
    audio_element_item_t *el_item;
    STAILQ_FOREACH(el_item, &pipeline->el_list, next) {
        if (el_item->linked) {
            audio_element_reset_input_ringbuf(el_item->el);
            audio_element_reset_output_ringbuf(el_item->el);
        }
    }
    return ESP_OK;
}
