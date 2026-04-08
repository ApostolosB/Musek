#include "ui_internal.h"

void _settings_open_cb(void *data, Evas_Object *parent, void *event_info);

typedef struct {
    Player_State *ps;
    Evas_Object  *popup;
    Evas_Object  *entry;
} Settings_Dialog_Data;

static void
_settings_cancel_cb(void *data, Evas_Object *obj, void *event_info)
{
    Settings_Dialog_Data *sd = data;
    evas_object_del(sd->popup);
    free(sd);
}

static void
_settings_save_cb(void *data, Evas_Object *obj, void *event_info)
{
    Settings_Dialog_Data *sd = data;

    const char *newdir = elm_object_text_get(sd->entry);
    if (sd->ps && newdir && newdir[0]) {
        free(sd->ps->settings->music_folder);
        sd->ps->settings->music_folder = strdup(newdir);
    }

    evas_object_del(sd->popup);
    free(sd);
}

void
_settings_open_cb(void *data, Evas_Object *parent, void *event_info)
{
    Player_State *ps = data;

    /* Popup centered in parent window */
    Evas_Object *popup = elm_popup_add(parent);
    elm_object_part_text_set(popup, "title,text", "Settings");
    elm_popup_align_set(popup, 0.5, 0.5);          /* center */
    elm_popup_allow_events_set(popup, EINA_TRUE);  /* ensure it gets input */

    Settings_Dialog_Data *sd = calloc(1, sizeof(Settings_Dialog_Data));
    sd->ps = ps;
    sd->popup = popup;

    /* Content box */
    Evas_Object *box = elm_box_add(popup);
    evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_box_horizontal_set(box, EINA_FALSE);
    evas_object_show(box);

    /* Label */
    Evas_Object *label = elm_label_add(box);
    elm_object_text_set(label, "Music directory:");
    evas_object_show(label);
    elm_box_pack_end(box, label);

    /* Entry */
    Evas_Object *entry = elm_entry_add(box);
    elm_entry_single_line_set(entry, EINA_TRUE);
    if (ps->settings && ps->settings->music_folder)
        elm_object_text_set(entry, ps->settings->music_folder);
    evas_object_size_hint_weight_set(entry, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(entry, EVAS_HINT_FILL, 0.0);
    evas_object_show(entry);
    elm_box_pack_end(box, entry);
    sd->entry = entry;

    /* Buttons row */
    Evas_Object *hbox = elm_box_add(box);
    elm_box_horizontal_set(hbox, EINA_TRUE);
    evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(hbox, 1.0, 0.0);
    evas_object_show(hbox);
    elm_box_pack_end(box, hbox);

    Evas_Object *btn_ok = elm_button_add(hbox);
    elm_object_text_set(btn_ok, "OK");
    evas_object_show(btn_ok);
    elm_box_pack_end(hbox, btn_ok);

    Evas_Object *btn_cancel = elm_button_add(hbox);
    elm_object_text_set(btn_cancel, "Cancel");
    evas_object_show(btn_cancel);
    elm_box_pack_end(hbox, btn_cancel);

    evas_object_smart_callback_add(btn_ok, "clicked", _settings_save_cb, sd);
    evas_object_smart_callback_add(btn_cancel, "clicked", _settings_cancel_cb, sd);

    /* Attach content to popup */
    elm_object_content_set(popup, box);

    /* Focus entry by default */
    elm_object_focus_set(entry, EINA_TRUE);

    evas_object_show(popup);
}
