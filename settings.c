/* Glib preferences & matched GTK+ dialog.
 * 
 * (c) 2011 by gatopeich, licensed under a Creative Commons Attribution 3.0
 * Unported License: http://creativecommons.org/licenses/by/3.0/
 * Briefly: Use it however suits you better and just give me due credit.
 *
 * TODO:
 * - Temp range selection.
 */

#include <gtk/gtk.h>
#define DEBUG 1

static gchar* pref_filename =  "gatotrayrc";
static GKeyFile* pref_file = NULL;

GdkColor fg_color, bg_color, iow_color;
GdkColor temp_min_color, temp_max_color, temp_gradient[100];
GdkColor freq_min_color, freq_max_color, freq_gradient[100];
typedef struct {
    const gchar* description;
    const gchar* preset;
    GdkColor* color;
} PrefColor;
PrefColor pref_colors[] = {
    { "Foreground", "black", &fg_color },
    { "Background", "white", &bg_color },
    { "I/O wait", "blue", &iow_color },
    { "Min frequency", "green", &freq_min_color },
    { "Max frequency", "red", &freq_max_color },
    { "Min temperature", "blue", &temp_min_color },
    { "Max temperature", "red", &temp_max_color },
};

// Decides whether the icon has a transparent background.
gboolean pref_transparent = TRUE;

// This is the command that is run when the tray icon is left click. Max length 255.
// gchar* pref_command = "xterm -bg '#222222' -title 'htop' -geometry '100x32+40+40' htop";
gchar* pref_command = "xterm -title 'top' -geometry '80x24+40+40' top";

// Called when a user preference is changed to recalculate color values.
void preferences_changed() {
    for (int i = 0; i < 100; i++) {
        freq_gradient[i].red   = (freq_min_color.red   * (99 - i) + freq_max_color.red   * i) / 99;
        freq_gradient[i].green = (freq_min_color.green * (99 - i) + freq_max_color.green * i) / 99;
        freq_gradient[i].blue  = (freq_min_color.blue  * (99 - i) + freq_max_color.blue  * i) / 99;
        temp_gradient[i].red   = (temp_min_color.red   * (99 - i) + temp_max_color.red   * i) / 99;
        temp_gradient[i].green = (temp_min_color.green * (99 - i) + temp_max_color.green * i) / 99;
        temp_gradient[i].blue  = (temp_min_color.blue  * (99 - i) + temp_max_color.blue  * i) / 99;
    }
}

// Called when the transparency option is changed.
void on_transparency_toggled(GtkToggleButton *togglebutton) {
    pref_transparent = gtk_toggle_button_get_active(togglebutton);
    preferences_changed();
}

void on_command_changed(GtkEntry *entry) {
    pref_command = g_strdup(gtk_entry_get_text(entry));
    preferences_changed();
}

void pref_init() {
    g_assert(!pref_file);
    pref_file = g_key_file_new();
    gchar* path = g_strconcat(g_get_user_config_dir(), "/", pref_filename, NULL);

    // Read preference file from path.
    if (g_key_file_load_from_file(pref_file, path, G_KEY_FILE_KEEP_COMMENTS, NULL)) {
        g_message("Loaded preferences from %s", path);
    }
    g_free(path);

    // Parse and set the colors from gaotrayrc "Colors" section.
    for (PrefColor* c = pref_colors; c < pref_colors + G_N_ELEMENTS(pref_colors); c++) {
        gchar* value = g_key_file_get_string(pref_file, "Colors", c->description, NULL);
        if (!value) {
            gdk_color_parse(c->preset, c->color);
        } else {
            gdk_color_parse(value, c->color);
            g_free(value);
        }
    }

    // Load the transparency option from gatotrayrc "Options" section.
    GError* gerror = NULL;
    gboolean transparency = g_key_file_get_boolean(pref_file, "Options", "Transparent Background", &gerror);
    if (!gerror) {
        pref_transparent = transparency;
    }

    // Load the command option from gatotrayrc "Options" section.
    gchar* value = g_key_file_get_string(pref_file, "Options", "Launch Command", NULL);
    // If we got a value for the command option then set it.
    if (value) {
        pref_command = g_strndup(value, 255);
    }
    preferences_changed();
}

void pref_save() {
    // Store all the color preferences under "Colors".
    for (PrefColor* c = pref_colors; c < pref_colors + G_N_ELEMENTS(pref_colors); c++) {
        gchar* color = gdk_color_to_string(c->color);
        g_key_file_set_string(pref_file, "Colors", c->description, color);
        g_free(color);
    }

    // Store the background transparency preference
    g_key_file_set_boolean(pref_file, "Options", "Transparent Background", pref_transparent);
    // Store the command preference.
    g_key_file_set_string(pref_file, "Options", "Launch Command", pref_command);

    // Write the new preferences file.
    gchar* data = g_key_file_to_data(pref_file, NULL, NULL);
    gchar* path = g_strconcat(g_get_user_config_dir(), "/", pref_filename, NULL);
    g_message("Saving preferences to %s.", path);
    g_file_set_contents(path, data, -1, NULL);
    g_free(path);
    g_free(data);
}

GtkWidget *pref_dialog = NULL;
// Called when the preferences dialog has received a response (user clicks save)
void pref_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    if (GTK_RESPONSE_ACCEPT == response_id) {
        pref_save();
    }
    gtk_widget_destroy(GTK_WIDGET(pref_dialog));
}

// Called when the preferences dialog is closed without saving.
void pref_destroyed() {
    pref_dialog = NULL;
}

// Show the popup preferences dialog.
void show_pref_dialog() {
    if (pref_dialog) {
        return;
    }

    pref_dialog = gtk_dialog_new_with_buttons("gatotray Settings", NULL, 0,
            GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, GTK_STOCK_SAVE,
            GTK_RESPONSE_ACCEPT, NULL);
    //~ gtk_window_set_icon_name(GTK_WINDOW(pref_dialog), GTK_STOCK_PREFERENCES);
    g_signal_connect(G_OBJECT(pref_dialog), "response", G_CALLBACK(pref_response), NULL);
    g_signal_connect(G_OBJECT(pref_dialog), "destroy", G_CALLBACK(pref_destroyed), NULL);

    GtkWidget *vb = gtk_dialog_get_content_area(GTK_DIALOG(pref_dialog));
    GtkWidget *hb = gtk_hbox_new(FALSE, 0);
    GtkWidget *wb = gtk_hbox_new(FALSE, 0); // full width text entry for command.
    gtk_container_add(GTK_CONTAINER(vb), hb);
    gtk_container_add(GTK_CONTAINER(vb), wb); // add command to main container.
    GtkWidget *frame = gtk_frame_new("Colors");
    gtk_container_add(GTK_CONTAINER(hb), frame);
    vb = gtk_vbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(frame), vb);

    // Add the color pickers to the color options frame.
    for (PrefColor* c = pref_colors; c < pref_colors + G_N_ELEMENTS(pref_colors); c++) {
        GtkWidget *hb = gtk_hbox_new(FALSE, 0);
        gtk_container_add(GTK_CONTAINER(vb), hb);
        gtk_container_add(GTK_CONTAINER(hb), gtk_label_new(c->description));
        GtkWidget *cbutton = gtk_color_button_new_with_color(c->color);
        g_signal_connect(G_OBJECT(cbutton), "color-set", G_CALLBACK(gtk_color_button_get_color), c->color);
        g_signal_connect_after(G_OBJECT(cbutton), "color-set", G_CALLBACK(preferences_changed), c);
        gtk_box_pack_start(GTK_BOX(hb), cbutton, FALSE, FALSE, 0);
    }

    frame = gtk_frame_new("Options");
    gtk_container_add(GTK_CONTAINER(hb), frame);
    vb = gtk_vbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(frame), vb);

    // Add the transparent background checkbox.
    GtkWidget *cbutton = gtk_check_button_new_with_label("Transparent Background");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cbutton), pref_transparent);
    g_signal_connect(G_OBJECT(cbutton), "toggled", G_CALLBACK(on_transparency_toggled), NULL);
    gtk_box_pack_start(GTK_BOX(vb), cbutton, FALSE, FALSE, 0);

    frame = gtk_frame_new("Command run when tray icon is clicked:"); // create command frame.
    gtk_container_add(GTK_CONTAINER(wb), frame); // add command frame to full-width container.
    vb = gtk_hbox_new(FALSE, 0); // create the text box for entering command.
    gtk_container_add(GTK_CONTAINER(frame), vb); // add the command text box to the command frame.

    GtkWidget *entry = gtk_entry_new_with_max_length(255); // new text entry widget.
    gtk_entry_set_text(GTK_ENTRY(entry), pref_command); // set text to loaded value of command.
    gtk_editable_set_editable(GTK_EDITABLE(entry), TRUE); // make it user editable.
    gtk_entry_set_visibility(GTK_ENTRY(entry), TRUE); // make it visible.

    g_signal_connect(G_OBJECT(entry), "changed", G_CALLBACK(on_command_changed), NULL);
    gtk_box_pack_start(GTK_BOX(vb), entry, TRUE, TRUE, 0);
    gtk_widget_show_all(GTK_WIDGET(pref_dialog));
}
