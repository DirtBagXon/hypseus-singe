#include <gtk/gtk.h>


void
on_setup_window_destroy                (GtkObject       *object,
                                        gpointer         user_data);

void
configure_game                         (GtkButton       *button,
                                        gpointer         user_data);

void
configure_ldp                          (GtkButton       *button,
                                        gpointer         user_data);

void
startdaphne                            (GtkButton       *button,
                                        gpointer         user_data);

void
on_load_config_button_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void
on_save_config_button_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void
on_reset_defaults_button_clicked       (GtkButton       *button,
                                        gpointer         user_data);

void
on_dl_window_destroy                   (GtkObject       *object,
                                        gpointer         user_data);

void
on_dl_ok_clicked                       (GtkButton       *button,
                                        gpointer         user_data);

void
on_dl_cancel_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_ldp_window_destroy                  (GtkObject       *object,
                                        gpointer         user_data);

void
on_ldp_ok_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_ldp_cancel_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_tq_window_destroy                   (GtkObject       *object,
                                        gpointer         user_data);

void
on_tq_ok_clicked                       (GtkButton       *button,
                                        gpointer         user_data);

void
on_tq_cancel_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_sdq_window_destroy                  (GtkObject       *object,
                                        gpointer         user_data);

void
on_sdq_ok_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_sdq_cancel_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_fileselection1_destroy              (GtkObject       *object,
                                        gpointer         user_data);

void
on_file_ok_button1_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_file_cancel_button1_clicked         (GtkButton       *button,
                                        gpointer         user_data);

void
on_cliff_window_destroy                (GtkObject       *object,
                                        gpointer         user_data);

void
on_cliff_ok_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_cliff_cancel_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_no_config_button_destroy            (GtkObject       *object,
                                        gpointer         user_data);

void
on_no_config_button_clicked            (GtkButton       *button,
                                        gpointer         user_data);
