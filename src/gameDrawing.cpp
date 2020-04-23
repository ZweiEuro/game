#include <gameDrawing.hpp>

namespace game_display_output
{
size_t frames = 0;

ALLEGRO_EVENT_QUEUE *event_queue_draw_thread = NULL;
ALLEGRO_EVENT_QUEUE *event_queue_display = NULL;

// timer for fps, thread to do that drawing
ALLEGRO_TIMER *timer_draw_thread = NULL;
ALLEGRO_THREAD *draw_thread_pointer = NULL;

//we switch from buffer to display when buffer is drawn
ALLEGRO_DISPLAY *disp = NULL;
ALLEGRO_BITMAP *buffer = NULL;

// for scaling
ALLEGRO_TRANSFORM transform;
float scale_factor_x;
float scale_factor_y;

// prototypes
void *draw_thread(ALLEGRO_THREAD *thr, void *arg);
bool draw_thread_init(ALLEGRO_EVENT_SOURCE *event_source);
void disp_pre_draw();
void disp_post_draw();

bool start(ALLEGRO_EVENT_SOURCE *event_source)
{
    must_init(draw_thread_init(event_source), "draw_thread_init()");
    return true;
}
void stop()
{
    al_join_thread(get_p_draw_thread(), NULL);
    al_destroy_timer(timer_draw_thread);
    al_destroy_event_queue(event_queue_draw_thread);

    al_destroy_bitmap(buffer);
    al_destroy_display(disp);
    al_destroy_event_queue(event_queue_display);

    fprintf(stderr, "Draw thread deinit\n");
}

ALLEGRO_DISPLAY *get_disp()
{
    return disp;
}

void disp_pre_draw()
{

    al_set_target_bitmap(buffer);
}

void disp_post_draw()
{

    al_set_target_backbuffer(disp);
    al_draw_scaled_bitmap(buffer, 0, 0, BUFFER_W, BUFFER_H, 0, 0, DISP_W, DISP_H, 0);

    al_flip_display();
}

ALLEGRO_THREAD *get_p_draw_thread()
{
    return draw_thread_pointer;
}

bool draw_thread_init(ALLEGRO_EVENT_SOURCE *event_source)
{

    // DISP INIT
    if (get_fullscreen())
        al_set_new_display_flags(ALLEGRO_FULLSCREEN_WINDOW);
    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST);

    disp = al_create_display(DISP_W, DISP_H);
    must_init(disp, "display");

    scale_factor_x = ((float)al_get_display_width(disp)) / BUFFER_W;
    scale_factor_y = ((float)al_get_display_height(disp)) / BUFFER_H;

    buffer = al_create_bitmap(DISP_W, DISP_W);
    must_init(buffer, "bitmap buffer");

    //al_set_target_bitmap(buffer);
    al_identity_transform(&transform);
    al_scale_transform(&transform, scale_factor_x, scale_factor_y);
    al_use_transform(&transform);

    // event for display actions
    event_queue_display = al_create_event_queue();
    must_init(event_queue_display, "display_event_queue");
    al_register_event_source(event_queue_display, al_get_display_event_source(disp));

    // THREAD INIT
    event_queue_draw_thread = al_create_event_queue();
    must_init(event_queue_draw_thread, "draw-thread-queue");

    timer_draw_thread = al_create_timer(1.0 / 60.0);
    must_init(timer_draw_thread, "draw-thread-timer");

    al_register_event_source(event_queue_draw_thread, al_get_timer_event_source(timer_draw_thread));
    al_register_event_source(event_queue_draw_thread, event_source); // Gstate informant

    draw_thread_pointer = al_create_thread(draw_thread, NULL);
    if (draw_thread_pointer)
    {
        //al_set_thread_should_stop(ret);
        al_set_window_title(get_disp(), "Tong"); // TODO do this

        al_start_thread(draw_thread_pointer);
        al_start_timer(timer_draw_thread);
        al_show_mouse_cursor(get_disp());

        return true;
    }
    else
    {
        draw_thread_pointer = NULL;
        return false;
    }
}

void *draw_thread(ALLEGRO_THREAD *thr, void *arg)
{
    fprintf(stderr, "Draw thread started\n");
    ALLEGRO_EVENT event;

    if (get_program_state() == THREAD_STATES::D_RESTART)
    {
        fprintf(stderr, "started draw thread from restart waiting for go ahead \n");
        while (get_program_state() == THREAD_STATES::D_RESTART)
        {
            al_wait_for_event(event_queue_draw_thread, &event);
        }
        fprintf(stderr, "Go ahead granted: draw thread \n");
    }

    while (get_program_state() != THREAD_STATES::D_EXIT && get_program_state() != THREAD_STATES::D_RESTART)
    {

        al_wait_for_event(event_queue_draw_thread, &event);

        switch (event.type)
        {
        case ALLEGRO_EVENT_TIMER:

            disp_pre_draw();
            al_clear_to_color(al_color_name("black"));
            al_draw_text(get_font(), al_color_name("white"), 1, 1, ALLEGRO_ALIGN_CENTER, "Hello world!");

            disp_post_draw();
            break;
        case G_STATE_CHANGE_EVENT_NUM:
            // check if we need to close the thread
            fprintf(stderr, "drawing thread got gamestate change %i \n", get_program_state());

            break;
        default:
            fprintf(stderr, "unexpected event in draw thread!>%i<\n", event.type);
            break;
        }
    }
    fprintf(stderr, "drawing thread exit\n");
    return NULL;
}

} // namespace game_display_output