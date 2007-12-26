#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include <stdio.h>

GtkWidget *g_cliff_window = NULL;
GtkWidget *g_dl_window = NULL;
GtkWidget *g_ldp_window = NULL;
GtkWidget *g_sdq_window = NULL;
GtkWidget *g_tq_window = NULL;
GtkWidget *g_no_config_dialog = NULL;
GtkWidget *g_fileselection1 = NULL;

struct cliff_settings
{
	int difficulty_index;
	int cost_index;
	int lives_index;
	int audio_index;
	int scene_length_index;
	int buy_in_index;
	int hanging_scene_index;
	int overlay_index;
	int action_stick_index;
	int hints_index;
};

struct dl_settings
{
	int difficulty_index;
	int lives_index;
	int audio_index;
	int cost_index;
	int scoreboard_index;
	int disc_index;
};

struct ldp_settings
{
	int port_index;
	int speed_index;
};

struct sdq_settings
{
	int difficulty_index;
	int lives_index;
	int audio_index;
	int cost_index;
	int extra_life_index;
};

struct tq_settings
{
	int time_index;
	int lives_index;
	int audio_index;
	int cost_index;
	int scoreboard_index;
};

struct cliff_settings g_cliff;
struct dl_settings g_dl;
struct ldp_settings g_ldp;
struct sdq_settings g_sdq;
struct tq_settings g_tq;

enum { GAME_CLIFF, GAME_DL, GAME_DLE, GAME_ACE, GAME_SPEEDTEST, GAME_SDQ, GAME_TQ };
enum { LDP_NONE, LDP_HITACHI, LDP_MPEG, LDP_PIONEER, LDP_V6000, LDP_SONY };

gint get_active_index(GtkButton *button, char *option_menu_name)
{
	GtkWidget *option_menu, *menu, *active_item;
	gint active_index;

	option_menu = lookup_widget (GTK_WIDGET (button), option_menu_name);
	menu = GTK_OPTION_MENU (option_menu)->menu;
	active_item = gtk_menu_get_active (GTK_MENU (menu));
	active_index = g_list_index (GTK_MENU_SHELL (menu)->children, active_item);

	return(active_index);
}

void
configure_game                         (GtkButton       *button,
                                        gpointer         user_data)
{
	gint active_index = get_active_index(button, "game_option_menu");

	switch (active_index)
	{
	case GAME_CLIFF:
		if (!g_cliff_window)
		{
			GtkWidget *option_menu = NULL;

			g_cliff_window = create_cliff_window();

			option_menu = lookup_widget (GTK_WIDGET (g_cliff_window), "cliff_difficulty");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_cliff.difficulty_index);
			option_menu = lookup_widget (GTK_WIDGET (g_cliff_window), "cliff_cost");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_cliff.cost_index);
			option_menu = lookup_widget (GTK_WIDGET (g_cliff_window), "cliff_lives");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_cliff.lives_index);
			option_menu = lookup_widget (GTK_WIDGET (g_cliff_window), "cliff_audio");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_cliff.audio_index);
			option_menu = lookup_widget (GTK_WIDGET (g_cliff_window), "cliff_scene_length");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_cliff.scene_length_index);
			option_menu = lookup_widget (GTK_WIDGET (g_cliff_window), "cliff_buy_in");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_cliff.buy_in_index);
			option_menu = lookup_widget (GTK_WIDGET (g_cliff_window), "cliff_hanging_scene");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_cliff.hanging_scene_index);
			option_menu = lookup_widget (GTK_WIDGET (g_cliff_window), "cliff_overlay");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_cliff.overlay_index);
			option_menu = lookup_widget (GTK_WIDGET (g_cliff_window), "cliff_action_stick");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_cliff.action_stick_index);
			option_menu = lookup_widget (GTK_WIDGET (g_cliff_window), "cliff_hints");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_cliff.hints_index);

			gtk_widget_show(g_cliff_window);
		}
		break;
	case GAME_DL:
	case GAME_DLE:
	case GAME_ACE:
		// we don't want to create/display the DL window if it is already displayed
		if (!g_dl_window)
		{
			GtkWidget *option_menu = NULL;
			
			g_dl_window = create_dl_window();

			if (active_index == GAME_ACE)
			{
				gtk_window_set_title (GTK_WINDOW (g_dl_window), _("Space Ace Configuration"));
			}

			option_menu = lookup_widget (GTK_WIDGET (g_dl_window), "dl_difficulty");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_dl.difficulty_index);
			option_menu = lookup_widget (GTK_WIDGET (g_dl_window), "dl_lives");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_dl.lives_index);
			option_menu = lookup_widget (GTK_WIDGET (g_dl_window), "dl_audio");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_dl.audio_index);
			option_menu = lookup_widget (GTK_WIDGET (g_dl_window), "dl_cost");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_dl.cost_index);
			option_menu = lookup_widget (GTK_WIDGET (g_dl_window), "dl_scoreboard");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_dl.scoreboard_index);
			option_menu = lookup_widget (GTK_WIDGET (g_dl_window), "dl_disc");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_dl.disc_index);

			gtk_widget_show(g_dl_window);
		}
		break;
	case GAME_SPEEDTEST:
		if (!g_no_config_dialog)
		{
			g_no_config_dialog = create_no_config_dialog();
			gtk_widget_show(g_no_config_dialog);
		}
		break;
	case GAME_SDQ:
		if (!g_sdq_window)
		{
			GtkWidget *option_menu = NULL;

			g_sdq_window = create_sdq_window();

			option_menu = lookup_widget (GTK_WIDGET (g_sdq_window), "sdq_difficulty");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_sdq.difficulty_index);
			option_menu = lookup_widget (GTK_WIDGET (g_sdq_window), "sdq_lives");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_sdq.lives_index);
			option_menu = lookup_widget (GTK_WIDGET (g_sdq_window), "sdq_audio");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_sdq.audio_index);
			option_menu = lookup_widget (GTK_WIDGET (g_sdq_window), "sdq_cost");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_sdq.cost_index);
			option_menu = lookup_widget (GTK_WIDGET (g_sdq_window), "sdq_extra_life");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_sdq.extra_life_index);

			gtk_widget_show(g_sdq_window);
		}
		break;
	case GAME_TQ:
		if (!g_tq_window)
		{
			GtkWidget *option_menu = NULL;

			g_tq_window = create_tq_window();

			option_menu = lookup_widget (GTK_WIDGET (g_tq_window), "tq_time");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_tq.time_index);
			option_menu = lookup_widget (GTK_WIDGET (g_tq_window), "tq_lives");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_tq.lives_index);
			option_menu = lookup_widget (GTK_WIDGET (g_tq_window), "tq_audio");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_tq.audio_index);
			option_menu = lookup_widget (GTK_WIDGET (g_tq_window), "tq_cost");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_tq.cost_index);
			option_menu = lookup_widget (GTK_WIDGET (g_tq_window), "tq_scoreboard");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_tq.scoreboard_index);

			gtk_widget_show(g_tq_window);
		}
		break;
	default:
		printf("Unknown game, BUG!\n");
		break;
	}
}


void
configure_ldp                          (GtkButton       *button,
                                        gpointer         user_data)
{
	gint active_index = get_active_index(button, "ldp_option_menu");

	switch(active_index)
	{
	case LDP_NONE:
		if (!g_no_config_dialog)
		{
			g_no_config_dialog = create_no_config_dialog();
			gtk_widget_show(g_no_config_dialog);
		}
		break;
	case LDP_HITACHI:
	case LDP_PIONEER:
	case LDP_V6000:
	case LDP_SONY:
		if (!g_ldp_window)
        {
			GtkWidget *option_menu = NULL;
			
			g_ldp_window = create_ldp_window();

			option_menu = lookup_widget (GTK_WIDGET (g_ldp_window), "ldp_port");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_ldp.port_index);
			option_menu = lookup_widget (GTK_WIDGET (g_ldp_window), "ldp_speed");
			gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), g_ldp.speed_index);
        
            gtk_widget_show(g_ldp_window);
        }
		break;
	case LDP_MPEG:
		break;
	}

}


void
startdaphne                            (GtkButton       *button,
                                        gpointer         user_data)
{
	gtk_main_quit();
}


void
on_dl_ok_clicked                       (GtkButton       *button,
                                        gpointer         user_data)
{
	gint active_index = 0;

	g_dl.difficulty_index = get_active_index(button, "dl_difficulty");
	g_dl.lives_index = get_active_index(button, "dl_lives");
	g_dl.audio_index = get_active_index(button, "dl_audio");
	g_dl.cost_index = get_active_index(button, "dl_cost");
	g_dl.scoreboard_index = get_active_index(button, "dl_scoreboard");
	g_dl.disc_index = get_active_index(button, "dl_disc");
	printf("DL ok\n");
	on_dl_cancel_clicked(NULL, 0);
}


void
on_dl_cancel_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	printf("DL cancelled\n");
	if (g_dl_window)
	{
		gtk_widget_destroy(g_dl_window);
	}
}


void
on_ldp_ok_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	g_ldp.port_index = get_active_index(button, "ldp_port");
	g_ldp.speed_index = get_active_index(button, "ldp_speed");
	on_ldp_cancel_clicked(NULL, 0);
}


void
on_ldp_cancel_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
	if (g_ldp_window)
	{
		gtk_widget_destroy(g_ldp_window);
	}
}


void
on_tq_ok_clicked                       (GtkButton       *button,
                                        gpointer         user_data)
{
	g_tq.time_index = get_active_index(button, "tq_time");
	g_tq.lives_index = get_active_index(button, "tq_lives");
	g_tq.audio_index = get_active_index(button, "tq_audio");
	g_tq.cost_index = get_active_index(button, "tq_cost");
	g_tq.scoreboard_index = get_active_index(button, "tq_scoreboard");
	on_tq_cancel_clicked(NULL, 0);
}


void
on_tq_cancel_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
	if (g_tq_window)
	{
		gtk_widget_destroy(g_tq_window);
	}
}


void
on_sdq_ok_clicked                      (GtkButton       *button,
                                        gpointer         user_data)
{
	g_sdq.difficulty_index = get_active_index(button, "sdq_difficulty");
	g_sdq.lives_index = get_active_index(button, "sdq_lives");
	g_sdq.audio_index = get_active_index(button, "sdq_audio");
	g_sdq.cost_index = get_active_index(button, "sdq_cost");
	g_sdq.extra_life_index = get_active_index(button, "sdq_extra_life");
	on_sdq_cancel_clicked(NULL, 0);
}


void
on_sdq_cancel_clicked                  (GtkButton       *button,
                                        gpointer         user_data)
{
	if (g_sdq_window)
	{
		gtk_widget_destroy(g_sdq_window);
	}
}


void
on_cliff_ok_clicked                    (GtkButton       *button,
                                        gpointer         user_data)
{

	g_cliff.difficulty_index = get_active_index(button, "cliff_difficulty");
	g_cliff.cost_index = get_active_index(button, "cliff_cost");
	g_cliff.lives_index = get_active_index(button, "cliff_lives");
	g_cliff.audio_index = get_active_index(button, "cliff_audio");
	g_cliff.scene_length_index = get_active_index(button, "cliff_scene_length");
	g_cliff.buy_in_index = get_active_index(button, "cliff_buy_in");
	g_cliff.hanging_scene_index = get_active_index(button, "cliff_hanging_scene");
	g_cliff.overlay_index = get_active_index(button, "cliff_overlay");
	g_cliff.action_stick_index = get_active_index(button, "cliff_action_stick");
	g_cliff.hints_index = get_active_index(button, "cliff_hints");

	on_cliff_cancel_clicked(NULL, 0);
}


void
on_cliff_cancel_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
	if (g_cliff_window)
	{
		gtk_widget_destroy(g_cliff_window);
	}
}

void
on_setup_window_destroy                (GtkObject       *object,
                                        gpointer         user_data)
{
	printf("setup destroy\n");
	gtk_main_quit();
}


void
on_dl_window_destroy                   (GtkObject       *object,
                                        gpointer         user_data)
{
	g_dl_window = NULL;
}


void
on_ldp_window_destroy                  (GtkObject       *object,
                                        gpointer         user_data)
{
	g_ldp_window = NULL;
}


void
on_tq_window_destroy                   (GtkObject       *object,
                                        gpointer         user_data)
{
	g_tq_window = NULL;
}


void
on_sdq_window_destroy                  (GtkObject       *object,
                                        gpointer         user_data)
{
	g_sdq_window = NULL;
}


void
on_cliff_window_destroy                (GtkObject       *object,
                                        gpointer         user_data)
{
	g_cliff_window = NULL;
}

void
on_load_config_button_clicked          (GtkButton       *button,
                                        gpointer         user_data)
{
	if (!g_fileselection1)
	{
		g_fileselection1 = create_fileselection1();
		gtk_file_selection_complete(GTK_FILE_SELECTION(g_fileselection1),
			"*.ini");
		gtk_widget_show(g_fileselection1);
	}
}


void
on_save_config_button_clicked          (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_reset_defaults_button_clicked       (GtkButton       *button,
                                        gpointer         user_data)
{

}


void
on_no_config_button_destroy            (GtkObject       *object,
                                        gpointer         user_data)
{
	g_no_config_dialog = NULL;
}

void
on_no_config_button_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
	if (g_no_config_dialog)
	{
		gtk_widget_destroy(g_no_config_dialog);
	}
}


void
on_file_ok_button1_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
	gchar *filename = NULL;

	printf("File ok\n");
	filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(g_fileselection1));
	printf("%s\n", filename);
	on_file_cancel_button1_clicked(NULL, 0);
}


void
on_file_cancel_button1_clicked         (GtkButton       *button,
                                        gpointer         user_data)
{
	printf("file cancel\n");
	if (g_fileselection1)
	{
		gtk_widget_destroy(g_fileselection1);
	}
}

void
on_fileselection1_destroy                (GtkObject       *object,
                                        gpointer         user_data)
{
	printf("file selection destroy\n");
	g_fileselection1 = NULL;
}
