/*
 * Rig
 *
 * Copyright (C) 2013  Intel Corporation.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <stdlib.h>

#include <glib.h>

#include <rut.h>

#include "rig-engine.h"
#include "rig-simulator-service.h"
#include "rig-pb.h"
#include "rig.pb-c.h"

static void
simulator__test (Rig__Simulator_Service *service,
                 const Rig__Query *query,
                 Rig__TestResult_Closure closure,
                 void *closure_data)
{
  Rig__TestResult result = RIG__TEST_RESULT__INIT;
  //RigSimulator *simulator = rig_pb_rpc_closure_get_connection_data (closure_data);

  g_return_if_fail (query != NULL);

  g_print ("Simulator Service: Test Query\n");

  closure (&result, closure_data);
}

static void
simulator__load (Rig__Simulator_Service *service,
                 const Rig__UI *ui,
                 Rig__LoadResult_Closure closure,
                 void *closure_data)
{
  Rig__LoadResult result = RIG__LOAD_RESULT__INIT;
  RigSimulator *simulator =
    rig_pb_rpc_closure_get_connection_data (closure_data);
  RigEngine *engine = simulator->engine;
  RigPBUnSerializer *unserializer;

  g_return_if_fail (ui != NULL);

  g_print ("Simulator: UI Load Request\n");

  unserializer = rig_pb_unserializer_new (engine);

  rig_pb_unserialize_ui (unserializer, ui, false);

  rig_pb_unserializer_destroy (unserializer);

  closure (&result, closure_data);
}

static void
simulator__run_frame (Rig__Simulator_Service *service,
                      const Rig__FrameSetup *setup,
                      Rig__RunFrameAck_Closure closure,
                      void *closure_data)
{
  Rig__RunFrameAck ack = RIG__RUN_FRAME_ACK__INIT;
  RigSimulator *simulator =
    rig_pb_rpc_closure_get_connection_data (closure_data);
  RigEngine *engine = simulator->engine;
  int i;

  g_return_if_fail (setup != NULL);

  g_print ("Simulator: Run Frame Request: n_events = %d\n",
           setup->n_events);

  if (setup->has_width && setup->has_height &&
      (engine->width != setup->width ||
       engine->height != setup->height))
    {
      rig_engine_resize (engine, setup->width, setup->height);
    }

  for (i = 0; i < setup->n_events; i++)
    {
      Rig__Event *pb_event = setup->events[i];
      RutStreamEvent *event;

      if (!pb_event->has_type)
        {
          g_warning ("Event missing type");
          continue;
        }

      event = g_slice_new (RutStreamEvent);


      switch (pb_event->type)
        {
        case RIG__EVENT__TYPE__POINTER_MOVE:
          event->pointer_move.state = simulator->button_state;
          break;

        case RIG__EVENT__TYPE__POINTER_DOWN:
        case RIG__EVENT__TYPE__POINTER_UP:

          event->pointer_button.state = simulator->button_state;

          event->pointer_button.x = simulator->last_pointer_x;
          event->pointer_button.y = simulator->last_pointer_y;

          if (pb_event->pointer_button->has_button)
            event->pointer_button.button = pb_event->pointer_button->button;
          else
            {
              g_warn_if_reached ();
              event->pointer_button.button = RUT_BUTTON_STATE_1;
            }
          break;

        case RIG__EVENT__TYPE__KEY_DOWN:
        case RIG__EVENT__TYPE__KEY_UP:

          if (pb_event->key->has_keysym)
            event->key.keysym = pb_event->key->keysym;
          else
            {
              g_warn_if_reached ();
              event->key.keysym = RUT_KEY_a;
            }

          if (pb_event->key->has_mod_state)
            event->key.mod_state = pb_event->key->mod_state;
          else
            {
              g_warn_if_reached ();
              event->key.mod_state = 0;
            }
          break;
        }

      switch (pb_event->type)
        {
        case RIG__EVENT__TYPE__POINTER_MOVE:
          event->type = RUT_STREAM_EVENT_POINTER_MOVE;

          if (pb_event->pointer_move->has_x)
            event->pointer_move.x = pb_event->pointer_move->x;
          else
            {
              g_warn_if_reached ();
              event->pointer_move.x = 0;
            }

          if (pb_event->pointer_move->has_y)
            event->pointer_move.y = pb_event->pointer_move->y;
          else
            {
              g_warn_if_reached ();
              event->pointer_move.y = 0;
            }

          simulator->last_pointer_x = event->pointer_move.x;
          simulator->last_pointer_y = event->pointer_move.y;

          g_print ("Event: Pointer move (%f, %f)\n",
                   event->pointer_move.x, event->pointer_move.y);
          break;
        case RIG__EVENT__TYPE__POINTER_DOWN:
          event->type = RUT_STREAM_EVENT_POINTER_DOWN;
          simulator->button_state |= event->pointer_button.button;
          event->pointer_button.state |= event->pointer_button.button;
          g_print ("Event: Pointer down\n");
          break;
        case RIG__EVENT__TYPE__POINTER_UP:
          event->type = RUT_STREAM_EVENT_POINTER_UP;
          simulator->button_state &= ~event->pointer_button.button;
          event->pointer_button.state &= ~event->pointer_button.button;
          g_print ("Event: Pointer up\n");
          break;
        case RIG__EVENT__TYPE__KEY_DOWN:
          event->type = RUT_STREAM_EVENT_KEY_DOWN;
          g_print ("Event: Key down\n");
          break;
        case RIG__EVENT__TYPE__KEY_UP:
          event->type = RUT_STREAM_EVENT_KEY_UP;
          g_print ("Event: Key up\n");
          break;
        }

      rut_shell_handle_stream_event (engine->shell, event);
    }

  rut_shell_queue_redraw (engine->shell);

  closure (&ack, closure_data);
}

static Rig__Simulator_Service rig_simulator_service =
  RIG__SIMULATOR__INIT(simulator__);


static void
handle_frontend_test_response (const Rig__TestResult *result,
                                void *closure_data)
{
  g_print ("Renderer test response received\n");
}

static void
simulator_peer_connected (PB_RPC_Client *pb_client,
                          void *user_data)
{
  ProtobufCService *frontend_service =
    rig_pb_rpc_client_get_service (pb_client);
  Rig__Query query = RIG__QUERY__INIT;

  rig__frontend__test (frontend_service, &query,
                       handle_frontend_test_response, NULL);
  g_print ("Simulator peer connected\n");
}

static void
simulator_peer_error_handler (PB_RPC_Error_Code code,
                              const char *message,
                              void *user_data)
{
  RigSimulator *simulator = user_data;

  g_warning ("Simulator peer error: %s", message);

  rig_simulator_service_stop (simulator);
}

void
rig_simulator_service_start (RigSimulator *simulator)
{
  simulator->simulator_peer =
    rig_rpc_peer_new (simulator->engine,
                      simulator->fd,
                      &rig_simulator_service.base,
                      (ProtobufCServiceDescriptor *)&rig__frontend__descriptor,
                      simulator_peer_error_handler,
                      simulator_peer_connected,
                      simulator);
}

void
rig_simulator_service_stop (RigSimulator *simulator)
{
  rut_refable_unref (simulator->simulator_peer);
  simulator->simulator_peer = NULL;

  /* For now we assume we would only stop the service due to an RPC
   * error and so we should quit this process... */
  exit (1);
}
