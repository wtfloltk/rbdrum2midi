/*
 * libusb rockband 2 drum interface
 * based on U.are.U 4000B fingerprint scanner.
 * Copyright (C) 2007 Daniel Drake <dsd@gentoo.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mididrum.h" 
#include "rbkit.h"
#include "rb1kit.h"
#include "ghkit.h"

static int find_rbdrum_device(MIDIDRUM* MIDI_DRUM, struct libusb_device_handle **devh)
{
    // TODO: Currently the i argument is ignored.
    //PS3 RB kit
    *devh = libusb_open_device_with_vid_pid(NULL, 0x12ba, 0x0210);
    if(*devh){
        MIDI_DRUM->kit=PS_ROCKBAND;
        return 0;
	}

    //xbox RB kit
    *devh = libusb_open_device_with_vid_pid(NULL, 0x1bad, 0x0003);
    if(*devh){
        MIDI_DRUM->kit=XB_ROCKBAND;
        return 0;
	}

    //Wìì RB kit??
    *devh = libusb_open_device_with_vid_pid(NULL, 0x1bad, 0x0005);      
    if(*devh){
        MIDI_DRUM->kit=WII_ROCKBAND;
        return 0;
	}

    //PS3 GH kit
    *devh = libusb_open_device_with_vid_pid(NULL, 0x12ba, 0x0120);
    if(*devh){
        MIDI_DRUM->kit=GUITAR_HERO;
    }
  
    return *devh ? 0 : -EIO;
}

void init_kit(MIDIDRUM* MIDI_DRUM)
{
    //initialize all values, just to be safe
    memset(MIDI_DRUM->buf_indx,0,NUM_DRUMS);
    memset(MIDI_DRUM->buf_mask,0,NUM_DRUMS);
    switch(MIDI_DRUM->kit)
    {
        case PS_ROCKBAND:
	    case XB_ROCKBAND:
	    case WII_ROCKBAND:
            init_rb_kit(MIDI_DRUM);
	        break;
        case PS_ROCKBAND1:
	    case XB_ROCKBAND1:
            init_rb1_kit(MIDI_DRUM);
	        break;
        case GUITAR_HERO:
            init_gh_kit(MIDI_DRUM);
	        break;
    }
}

void print_hits(MIDIDRUM* MIDI_DRUM)
{
    if ( MIDI_DRUM->drum_state[RED] ||  
         MIDI_DRUM->drum_state[YELLOW] ||
  	 MIDI_DRUM->drum_state[BLUE] ||
    	 MIDI_DRUM->drum_state[GREEN] ||
    	 MIDI_DRUM->drum_state[ORANGE_CYMBAL] || 
	 MIDI_DRUM->drum_state[YELLOW_CYMBAL] ||  
	 MIDI_DRUM->drum_state[ORANGE_BASS]   ||
	 MIDI_DRUM->drum_state[BLACK_BASS] ) {
        printf("%s %s %s %s %s %s %s %s\n",  MIDI_DRUM->drum_state[RED]>0?"VV":"  ",
                                    MIDI_DRUM->drum_state[YELLOW]>0?"VV":"  ", 
    				    MIDI_DRUM->drum_state[BLUE]>0?"VV":"  ", 
    				    MIDI_DRUM->drum_state[GREEN]>0?"VV":"  ",
    				    MIDI_DRUM->drum_state[ORANGE_CYMBAL]>0?"VV":"  ",
                                    MIDI_DRUM->drum_state[YELLOW_CYMBAL]>0?"VV":"  ",
    				    MIDI_DRUM->drum_state[ORANGE_BASS]>0?"VV":"  ",
    				    MIDI_DRUM->drum_state[BLACK_BASS]>0?"VV":"  "); 
        printf("%02i %02i %02i %02i %02i %02i %02i %02i\n", MIDI_DRUM->drum_state[RED],
                                              MIDI_DRUM->drum_state[YELLOW],
    					      MIDI_DRUM->drum_state[BLUE], 
    					      MIDI_DRUM->drum_state[GREEN], 
    					      MIDI_DRUM->drum_state[ORANGE_CYMBAL],
                                              MIDI_DRUM->drum_state[YELLOW_CYMBAL], 
    					      MIDI_DRUM->drum_state[ORANGE_BASS],
					      MIDI_DRUM->drum_state[BLACK_BASS]);
    }
}

void print_buf(MIDIDRUM* MIDI_DRUM)
{
    if ( memcmp(MIDI_DRUM->oldbuf,MIDI_DRUM->buf,INTR_LENGTH))
    {
        printf("%02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x kit type=%d\n",
               MIDI_DRUM->buf[0], MIDI_DRUM->buf[1], MIDI_DRUM->buf[2], MIDI_DRUM->buf[3], MIDI_DRUM->buf[4],
	       MIDI_DRUM->buf[5], MIDI_DRUM->buf[6], MIDI_DRUM->buf[7], MIDI_DRUM->buf[8], MIDI_DRUM->buf[9],
               MIDI_DRUM->buf[10], MIDI_DRUM->buf[11], MIDI_DRUM->buf[12], MIDI_DRUM->buf[13], MIDI_DRUM->buf[14], 
	       MIDI_DRUM->buf[15], MIDI_DRUM->buf[16], MIDI_DRUM->buf[17], MIDI_DRUM->buf[18], MIDI_DRUM->buf[19],
               MIDI_DRUM->buf[20], MIDI_DRUM->buf[21], MIDI_DRUM->buf[22], MIDI_DRUM->buf[23], MIDI_DRUM->buf[24], 
	       MIDI_DRUM->buf[25], MIDI_DRUM->buf[26],MIDI_DRUM->kit);
	memcpy(MIDI_DRUM->oldbuf,MIDI_DRUM->buf,INTR_LENGTH);
    }
}

//debug mode callback
static void cb_irq_dbg(struct libusb_transfer *transfer)
{
    MIDIDRUM* MIDI_DRUM = (MIDIDRUM*)transfer->user_data; 
    if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
        fprintf(stderr, "irq transfer status %d? %d\n", transfer->status, LIBUSB_TRANSFER_ERROR);
        do_exit = 2;
        libusb_free_transfer(transfer);
        transfer = NULL;
        return;
    }

    print_buf(MIDI_DRUM);
    if (libusb_submit_transfer(transfer) < 0)
        do_exit = 2;
}

static int init_capture(struct libusb_transfer *irq_transfer)
{
    int r;

    r = libusb_submit_transfer(irq_transfer);
    if (r < 0)
        return r;

    return 6;
}

static int alloc_transfers(MIDIDRUM* MIDI_DRUM, libusb_device_handle *devh, struct libusb_transfer **irq_transfer)
{
    *irq_transfer = libusb_alloc_transfer(0);
    if (!*irq_transfer)
        return -ENOMEM;

    if(MIDI_DRUM->dbg){
        libusb_fill_interrupt_transfer(*irq_transfer, devh, EP_INTR, MIDI_DRUM->irqbuf,
            sizeof(MIDI_DRUM->irqbuf), cb_irq_dbg, (void*)MIDI_DRUM, 0);
        if( MIDI_DRUM->verbose)printf("Debug Mode Enabled..\n");
    }
    else if(MIDI_DRUM->kit == PS_ROCKBAND  || MIDI_DRUM->kit == XB_ROCKBAND ||
            MIDI_DRUM->kit == WII_ROCKBAND){
        libusb_fill_interrupt_transfer(*irq_transfer, devh, EP_INTR, MIDI_DRUM->irqbuf,
            sizeof(MIDI_DRUM->irqbuf), cb_irq_rb, (void*)MIDI_DRUM, 0);
        if( MIDI_DRUM->verbose)printf("Rock Band drum kit detected.\n");
    }
    else if(MIDI_DRUM->kit == PS_ROCKBAND1 || MIDI_DRUM->kit == XB_ROCKBAND1){
        libusb_fill_interrupt_transfer(*irq_transfer, devh, EP_INTR, MIDI_DRUM->irqbuf,
            sizeof(MIDI_DRUM->irqbuf), cb_irq_rb1, (void*)MIDI_DRUM, 0);
        if( MIDI_DRUM->verbose)printf("Rock Band 1 drum kit detected.\n");
    }
    else if(MIDI_DRUM->kit == GUITAR_HERO){
        libusb_fill_interrupt_transfer(*irq_transfer, devh, EP_INTR, MIDI_DRUM->irqbuf,
            sizeof(MIDI_DRUM->irqbuf), cb_irq_gh, (void*)MIDI_DRUM, 0);
        if( MIDI_DRUM->verbose)printf("Guitar Hero World Tour drum kit detected.\n");
    }
    else{
        printf("error in drum type! %i\n",MIDI_DRUM->kit);
    }

    return 0;
}

/*
These functions are from the official alsa docs:
http://www.alsa-project.org/alsa-doc/alsa-lib/seq.html
*/

// create a new client
snd_seq_t *open_client()
{
    snd_seq_t *handle;
    int err;
    err = snd_seq_open(&handle, "default", SND_SEQ_OPEN_OUTPUT, 0);
    if (err < 0)
        return NULL;
    snd_seq_set_client_name(handle, "Game Drumkit Client");
    return handle;
}

// create a new port; return the port id
// port will be writable and accept the write-subscription.
int my_new_port(snd_seq_t *handle)
{
    // |SND_SEQ_PORT_CAP_WRITE||SND_SEQ_PORT_CAP_SUBS_WRITE
    return snd_seq_create_simple_port(handle, "Game Drumkit port 2",
        SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
        SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);
}

// create a queue and return its id
int my_queue(snd_seq_t *handle)
{
    return snd_seq_alloc_named_queue(handle, "Game Drumkit queue");
}

// set the tempo and pulse per quarter note
void set_tempo(snd_seq_t *handle, int q)
{
    snd_seq_queue_tempo_t *tempo;
    snd_seq_queue_tempo_alloca(&tempo);
    perror("snd_seq_queue_tempo_alloca");
    snd_seq_queue_tempo_set_tempo(tempo, 1000000); // 60 BPM
    perror("snd_seq_queue_tempo_set_tempo");
    snd_seq_queue_tempo_set_ppq(tempo, 48); // 48 PPQ
    perror("snd_seq_queue_tempo_set_ppq");
    snd_seq_set_queue_tempo(handle, q, tempo);
    perror("snd_seq_set_queue_tempo");
}

// change the tempo on the fly
int change_tempo(snd_seq_t *handle, int my_client_id, int my_port_id, int q, unsigned int tempo)
{
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    ev.dest.client = SND_SEQ_CLIENT_SYSTEM;
    ev.dest.port = SND_SEQ_PORT_SYSTEM_TIMER;
    ev.source.client = my_client_id;
    ev.source.port = my_port_id;
    ev.queue = SND_SEQ_QUEUE_DIRECT; // no scheduling
    ev.data.queue.queue = q;        // affected queue id
    ev.data.queue.param.value = tempo;    // new tempo in microsec.
    return snd_seq_event_output(handle, &ev);
}

static void program_change(snd_seq_t *seq, int port, int chan, int program)
{
    snd_seq_event_t ev;

    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_source(&ev, port);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);
    //... // set event type, data, so on..
    //set_event_time(&ev, Mf_currtime);
    //snd_seq_ev_schedule_tick(&ev, q, 0, Mf_currtime);

    snd_seq_ev_set_pgmchange(&ev, chan, program);

    int rc = snd_seq_event_output(seq, &ev);
    if (rc < 0) {
        printf("written = %i (%s)\n", rc, snd_strerror(rc));
    }
    snd_seq_drain_output(seq);
}

// A lot easier:
void subscribe_output(snd_seq_t *seq, int client, int port)
{
    snd_seq_connect_to(seq, DEFAULT_CHANNEL, client, port);
}

// From test/playmidi1.c from alsa-lib-1.0.3.

// Direct delivery seems like what I'm doing..
inline void notedown(snd_seq_t *seq, int port, int chan, int pitch, int vol)
{
    snd_seq_event_t ev;

    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_source(&ev, port);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);
    //... // set event type, data, so on..
    //set_event_time(&ev, Mf_currtime);
    //snd_seq_ev_schedule_tick(&ev, q, 0, Mf_currtime);

    snd_seq_ev_set_noteon(&ev, chan, pitch, vol);

    int rc = snd_seq_event_output(seq, &ev);
    if (rc < 0) {
        printf("written = %i (%s)\n", rc, snd_strerror(rc));
    }
    snd_seq_drain_output(seq);
}

// When the note up, note off.
inline void noteup(snd_seq_t *seq, int port, int chan, int pitch, int vol)
{
    snd_seq_event_t ev;

    snd_seq_ev_clear(&ev);
    snd_seq_ev_set_source(&ev, port);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);
    //... // set event type, data, so on..

    snd_seq_ev_set_noteoff(&ev, chan, pitch, vol);

    snd_seq_event_output(seq, &ev);
    snd_seq_drain_output(seq);
}

int setup_alsa(snd_seq_t **seq, int *port, unsigned char verbose)
{
    if ( verbose) printf("Setting up alsa\n");

    *seq = open_client();
    if (*seq == NULL) {
        if ( verbose >= 0) printf("Error: open_client failed: %s\n", snd_strerror((int)seq));
        return 0;
    }

    int my_client_id = snd_seq_client_id(*seq);
    *port = my_new_port(*seq);
    if ( verbose) printf("client:port = %i:%i\n", my_client_id, *port);

    program_change(*seq, *port, DEFAULT_CHANNEL, 0);
    int ret = 1;
    notedown(*seq, *port, DEFAULT_CHANNEL, 57, 55);

    if ( verbose) printf("Returning %i\n", ret);
    return ret;
}

void testAlsa(snd_seq_t *seq, int port)
{
    notedown(seq, port, DEFAULT_CHANNEL, 57, 127);
    usleep(1000000);
    noteup(seq, port, DEFAULT_CHANNEL, 57, 0);
    usleep(1000000);

}

static void sighandler(int signum)
{
    do_exit = 1;
}

void useage()
{
    printf("rbdrum2midi a rockband/guitar hero drumset driver in userland\n");
    printf("\n");
    printf("\n");
    printf("USEAGE:\n");
    printf("    rbdrum2midi [-option <value>...]\n");
    printf("\n");
    printf("OPTIONS:\n");
    printf("    -v                          verbose mode\n");
    printf("    -r/y/b/g <value>            set midi note for -color of drum head\n");
    printf("    -ocy/ycy/bcy/gcy <value>    set midi note for -color of cymbal\n");
    printf("    -ob/bkb <value>             set midi note for -color bass pedal\n");
    printf("    -rb1                        specify rockband 1 drumset\n");
    printf("    -vel <value>                set default note velocity (for rb1 or bass)\n");
    printf("    -c <value>                  set midi channel to send notes on\n");
    printf("    -htdm <value>               set hihat color i.e, r/y.../bcy/gcy\n");
    printf("    -htp <value>                set hihat pedal color i.e. ob/bkb*\n"); 
    printf("    -hto <value>                set open hihat midi value of hihat mode drum\n");
    printf("    -htc <value>                set closed hihat midi value of hihat mode drum\n");
    printf("    -dbg                        debug mode (no midi output)\n");
    printf("    -h                          show this message\n");
    printf("\n");
    printf("EXAMPLES:\n");
    printf("    rbdrum2midi -r 16 -bcy 64 -ob 32 -g 17 \n");
    printf("    rbdrum2midi -bkb 0 -htp bkb -htdm ycy -hto 46 -htc 42\n");
    printf("    rbdrum2midi -v -rb1\n");
    printf("\n");
    printf("NOTES:\n");
    printf("    r=red, o=orange, y=yellow, g=green, b=blue, bk=black\n");
    printf("    cy=cymbal, b=bass pedal, ht=hihat, otherwise drum pad\n");
    printf("    the default midi values are for the hydrogen yamaha vintage kit\n\n");
    printf("    *using -htp option sets pedal in \"hihat mode,\" allows users to play\n");
    printf("     different notes on drum specified by -htdm dependent on pedal state\n"); 
    printf("     default is hat mode on black bass, use -htp 0 to disable\n"); 
    printf("     hihat mode can also play a sound when the pedal is closed\n");
    printf("     if you don't want the pedal sound, don't specify note for pedal\n");
    printf("\n");

    return;
}

int main(int argc, char **argv)
{
    struct sigaction sigact;
    struct libusb_device_handle *devh;
    struct libusb_transfer *irq_transfer;
    int r = 1;
    int i = 0;
    MIDIDRUM MIDI_DRUM_;
    MIDIDRUM* MIDI_DRUM = &MIDI_DRUM_;

    //struct ALSA_MIDI_SEQUENCER seq;//currently only alsa supported
    //MIDI_DRUM->sequencer = (void*)&seq;
    
    MIDI_DRUM->buf = MIDI_DRUM->irqbuf;
    //initial conditions, defaults
    MIDI_DRUM->bass_down = 0;
    MIDI_DRUM->default_velocity = 125;
    MIDI_DRUM->channel = DEFAULT_CHANNEL;
    MIDI_DRUM->verbose = 0;
    MIDI_DRUM->dbg = 0;
    MIDI_DRUM->kit = PS_ROCKBAND;
    MIDI_DRUM->hat_mode = BLACK_BASS;
    MIDI_DRUM->hat = YELLOW_CYMBAL;
    memset(MIDI_DRUM->oldbuf,0,INTR_LENGTH);
    memset(MIDI_DRUM->drum_state,0,NUM_DRUMS);
    memset(MIDI_DRUM->prev_state,0,NUM_DRUMS);
    
    //default midi values;
    MIDI_DRUM->midi_note[RED] = YVK_SNARE; 
    MIDI_DRUM->midi_note[YELLOW] = YVK_HI_TOM;
    MIDI_DRUM->midi_note[BLUE] = YVK_MID_TOM;
    MIDI_DRUM->midi_note[GREEN] = YVK_LO_TOM;
    MIDI_DRUM->midi_note[YELLOW_CYMBAL] = YVK_OPEN_HAT;
    MIDI_DRUM->midi_note[GREEN_CYMBAL] = YVK_CRASH;
    MIDI_DRUM->midi_note[BLUE_CYMBAL] = YVK_RIDE;
    MIDI_DRUM->midi_note[ORANGE_CYMBAL] = YVK_CRASH;
    MIDI_DRUM->midi_note[ORANGE_BASS] = YVK_KICK;
    MIDI_DRUM->midi_note[BLACK_BASS] = 0;
    MIDI_DRUM->midi_note[OPEN_HAT] = YVK_OPEN_HAT;
    MIDI_DRUM->midi_note[CLOSED_HAT] = YVK_CLOSED_HAT;

    if (argc > 1) {
        for (i = 1;i<argc;i++)
        {
            if (strcmp(argv[i], "-v") == 0) {
                 MIDI_DRUM->verbose = 1;
            }
            else if (strcmp(argv[i], "-rb1") == 0) {
                //rockband 1 set, use different irq routine
                MIDI_DRUM->kit = PS_ROCKBAND1;
            }
            else if (strcmp(argv[i], "-ocy") == 0) {
                //orange cymbal
                MIDI_DRUM->midi_note[ORANGE_CYMBAL] = atoi(argv[++i]);
            }
            else if (strcmp(argv[i], "-ycy") == 0) {
                //yellow cymbal
                MIDI_DRUM->midi_note[YELLOW_CYMBAL] = atoi(argv[++i]);
            }
            else if (strcmp(argv[i], "-gcy") == 0) {
                //green cymbal
                MIDI_DRUM->midi_note[GREEN_CYMBAL] = atoi(argv[++i]);
            }
            else if (strcmp(argv[i], "-bcy") == 0) {
                //blue cymbal
                MIDI_DRUM->midi_note[BLUE_CYMBAL] = atoi(argv[++i]);
            }
            else if (strcmp(argv[i], "-ob") == 0) {
                //orange bass
                MIDI_DRUM->midi_note[ORANGE_BASS] = atoi(argv[++i]);
            }
            else if (strcmp(argv[i], "-bkb") == 0) {
                //black bass
                MIDI_DRUM->midi_note[BLACK_BASS] = atoi(argv[++i]);
            }
            else if (strcmp(argv[i], "-r") == 0) {
                //red pad
                MIDI_DRUM->midi_note[RED] = atoi(argv[++i]);
            }
            else if (strcmp(argv[i], "-y") == 0) {
                //yellow pad
                MIDI_DRUM->midi_note[YELLOW] = atoi(argv[++i]);
            }
            else if (strcmp(argv[i], "-g") == 0) {
                //green pad
                MIDI_DRUM->midi_note[GREEN] = atoi(argv[++i]);
            }
            else if (strcmp(argv[i], "-b") == 0) {
                //blue pad
                 MIDI_DRUM->midi_note[BLUE] = atoi(argv[++i]);
            }
	    else if (strcmp(argv[i], "-vel") == 0) {
	         MIDI_DRUM->default_velocity = min(max(atoi(argv[++i]),1),127); 
	    }
	    else if (strcmp(argv[i], "-c") == 0) {
	         MIDI_DRUM->channel = min(max(atoi(argv[++i]),0),15); 
	    }
	    else if (strcmp(argv[i], "-htp") == 0){
	         if (strcmp(argv[++i], "0" ) == 0)
		     MIDI_DRUM->hat_mode = 0; 
                 else if (strcmp(argv[i], "ob") == 0){
                     if(MIDI_DRUM->midi_note[ORANGE_BASS] = YVK_KICK)
                         MIDI_DRUM->midi_note[ORANGE_BASS] = 0;
		     MIDI_DRUM->hat_mode = ORANGE_BASS;
		 }
                 else if (strcmp(argv[i], "bkb") == 0) 
		     MIDI_DRUM->hat_mode = BLACK_BASS;
		 else {
		     printf("ERROR! Unknown pedal for hi-hat! Using default black bass");
		     MIDI_DRUM->hat_mode = BLACK_BASS; 
		 }
	    }
	    else if (strcmp(argv[i], "-htdm") == 0){
                 if (strcmp(argv[++i], "ocy") == 0) 
		     MIDI_DRUM->hat = ORANGE_CYMBAL;
                 else if (strcmp(argv[i], "ycy") == 0) 
		     MIDI_DRUM->hat = YELLOW_CYMBAL;
                 else if (strcmp(argv[i], "gcy") == 0) 
		     MIDI_DRUM->hat = GREEN_CYMBAL;
                 else if (strcmp(argv[i], "bcy") == 0) 
		     MIDI_DRUM->hat = BLUE_CYMBAL;
                 else if (strcmp(argv[i], "r") == 0) 
		     MIDI_DRUM->hat = RED;
                 else if (strcmp(argv[i], "y") == 0) 
		     MIDI_DRUM->hat = YELLOW;
                 else if (strcmp(argv[i], "b") == 0) 
		     MIDI_DRUM->hat = RED;
                 else if (strcmp(argv[i], "g") == 0) 
		     MIDI_DRUM->hat = GREEN;
		 else{
		     printf("ERROR! Unknown drum for hi-hat! Using default yellow cymbal");
                     MIDI_DRUM->hat = YELLOW_CYMBAL;
		 }
	    }
	    else if (strcmp(argv[i], "-hto") == 0){
	         MIDI_DRUM->midi_note[OPEN_HAT] = atoi(argv[++i]);
	    }
	    else if (strcmp(argv[i], "-htc") == 0){ 
	         MIDI_DRUM->midi_note[CLOSED_HAT] = atoi(argv[++i]);
            }
	    else if (strcmp(argv[i], "-dbg") == 0) {
                //debug mode
                MIDI_DRUM->dbg = 1;
		MIDI_DRUM->verbose = 1;
            }
            else if (strcmp(argv[i], "-h") == 0) {
                //help
                useage();
            }
            else{
                printf("Unknown argument! %s\n",argv[i]);
                useage();
            }
            //i = atoi(argv[1]);
        }
    }
    if(MIDI_DRUM->hat_mode)
    {
        MIDI_DRUM->midi_note[MIDI_DRUM->hat] = MIDI_DRUM->midi_note[OPEN_HAT];
    }
    r = libusb_init(NULL);
    if (r < 0) {
        fprintf(stderr, "failed to initialise libusb\n");
        exit(1);
    }
    if(MIDI_DRUM->kit==PS_ROCKBAND1){
        //no way of knowing if device is RB1 so reassign kit after claiming
        r = find_rbdrum_device(MIDI_DRUM,&devh);
        switch(MIDI_DRUM->kit)
	{
	    case PS_ROCKBAND:
	    case WII_ROCKBAND:
  	        MIDI_DRUM->kit = PS_ROCKBAND1; 
	        break;
	    case XB_ROCKBAND:
	        MIDI_DRUM->kit = XB_ROCKBAND1; 
	        break;
	}
    }
    else
        r = find_rbdrum_device(MIDI_DRUM,&devh);
    if (r < 0) {
        fprintf(stderr, "Could not find/open device\n");
        libusb_close(devh);
        libusb_exit(NULL);
        return -r;
    }
    init_kit(MIDI_DRUM);

    if (libusb_kernel_driver_active(devh, 0)) {
        r = libusb_detach_kernel_driver(devh, 0);
        if (r < 0) {
            printf("did not detach.\n");
        }
    }
    r = libusb_claim_interface(devh, 0);
    if (r < 0) {
        fprintf(stderr, "usb_claim_interface error %d %d\n", r, LIBUSB_ERROR_BUSY);
        libusb_close(devh);
        libusb_exit(NULL);
        return -r;
    }
    printf("claimed interface\n");

    int ret = setup_alsa(& MIDI_DRUM->g_seq, & MIDI_DRUM->g_port, MIDI_DRUM->verbose);
    // 0 is fail.
    if (ret == 0) {
        printf("Error: Alsa setup failed.\n");
        return 1;
    }

    /* async from here onwards */

    r = alloc_transfers(MIDI_DRUM, devh, &irq_transfer);
    if (r < 0) {
        // Deinit & Release
        libusb_free_transfer(irq_transfer);
        libusb_release_interface(devh, 0);
        libusb_close(devh);
        libusb_exit(NULL);
        snd_seq_close( MIDI_DRUM->g_seq);
        printf("alloc_transfers failed.\n");
        return -r;
    }

    r = init_capture(irq_transfer);
    if (r < 0) {
        // Deinit & Release
        libusb_free_transfer(irq_transfer);
        libusb_release_interface(devh, 0);
        libusb_close(devh);
        libusb_exit(NULL);
        snd_seq_close( MIDI_DRUM->g_seq);
        printf("init_capture failed.\n");
        return -r;
    }

    sigact.sa_handler = sighandler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_RESTART;
    r = sigaction(SIGINT, &sigact, NULL);
    r = sigaction(SIGTERM, &sigact, NULL);
    r = sigaction(SIGQUIT, &sigact, NULL);

    while (!do_exit) {
        r = libusb_handle_events(NULL);
        if (r < 0) {
            break;
        }
    }

    printf("shutting down...\n");

    if (irq_transfer) {
        r = libusb_cancel_transfer(irq_transfer);
        if (r < 0) {
            // Deinit & Release
            libusb_free_transfer(irq_transfer);
            libusb_release_interface(devh, 0);
            libusb_close(devh);
            libusb_exit(NULL);
            snd_seq_close( MIDI_DRUM->g_seq);
            printf("libusb_cancel_transfer failed.\n");
            return -r;
        }
    }

//leftover transfers are handled in callbacks
    // || img_transfer
    while (do_exit!=2)//(irq_transfer)
        if (libusb_handle_events(NULL) < 0)
            break;


    if (do_exit == 1)
        r = 0;
    else
        r = 1;

//out_deinit: 
//    libusb_free_transfer(irq_transfer); 
//out_release:
    libusb_release_interface(devh, 0);
//out:
    libusb_close(devh);
    libusb_exit(NULL);
    snd_seq_close( MIDI_DRUM->g_seq);
    
    return r >= 0 ? r : -r;
}

