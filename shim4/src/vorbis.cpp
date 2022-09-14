#ifdef USE_VORBIS

// TMG
// Ripped entirely from decode_example.c in libvorbis source!

/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2009             *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

function: simple example decoder
last mod: $Id: decoder_example.c 16243 2009-07-10 02:49:31Z xiphmont $

 ********************************************************************/

/* Takes a vorbis bitstream from stdin and writes raw stereo PCM to
   stdout. Decodes simple and chained OggVorbis files from beginning
   to end. Vorbisfile.a is somewhat more complex than the code below.  */

/* Note that this is POSIX, not ANSI code */

#include "shim4/main.h"
#include "shim4/util.h"

using namespace noo;

namespace noo {

namespace audio {

#include <vorbis/codec.h>

#ifdef USE_VORBISFILE
#include <vorbis/vorbisfile.h>

static size_t sdl_vorbis_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	return SDL_RWread((SDL_RWops *)datasource, ptr, size, nmemb);
}

static int sdl_vorbis_seek(void *datasource, ogg_int64_t offset, int whence)
{
	switch (whence) { // from Allegro, changed to SDL
		case SEEK_SET: whence = RW_SEEK_SET; break;
		case SEEK_CUR: whence = RW_SEEK_CUR; break;
		case SEEK_END: whence = RW_SEEK_END; break;
	}

	return (int)SDL_RWseek((SDL_RWops *)datasource, offset, whence);
}

static long sdl_vorbis_tell(void *datasource)
{
	return (long)SDL_RWtell((SDL_RWops *)datasource);
}

Uint8 *decode_vorbis(SDL_RWops *file, char *errmsg, SDL_AudioSpec *spec)
{
	char pcmout[4096];

	ov_callbacks callbacks;
	callbacks.read_func = sdl_vorbis_read;
	callbacks.seek_func = sdl_vorbis_seek;
	callbacks.close_func = 0;
	callbacks.tell_func = sdl_vorbis_tell;

	OggVorbis_File vf;
	int eof=0;
	int current_section;

	if (ov_open_callbacks(file, &vf, NULL, 0, callbacks) < 0) {
		strcpy(errmsg,"Input does not appear to be an Ogg bitstream.\n");
		return 0;
	}

	vorbis_info *vi=ov_info(&vf,-1);
	long total_samples = (long)ov_pcm_total(&vf,-1);

	spec->freq = (int)vi->rate;
	spec->channels = vi->channels;
	spec->size = int(total_samples * 2/*16 bit samples*/ * vi->channels);
	spec->format = AUDIO_S16; // FIXME!
	spec->silence = 0;
	spec->samples = 4096;
	spec->callback = 0;
	spec->userdata = 0;

	Uint8 *data = new Uint8[spec->size];

	int count = 0;

	while(!eof){
		long ret=ov_read(&vf,pcmout,sizeof(pcmout),0,2,1,&current_section);
		if (ret == 0) {
			/* EOF */
			eof=1;
		} else if (ret < 0) {
			/* error in the stream.  Not a problem, just reporting it in
			case we (the app) cares.  In this case, we don't. */
		} else {
			/* we don't bother dealing with sample rate changes, etc, but
			you'll have to*/
			memcpy(data+count, pcmout, ret);
			count += ret;
		}
	}

	ov_clear(&vf);

	return data;
}
#else
ogg_int16_t convbuffer[4096]; /* take 8k out of the data segment, not the stack */
int convsize=4096;

Uint8 *decode_vorbis(SDL_RWops *file, char *errmsg, SDL_AudioSpec *spec)
{
    ogg_sync_state   oy; /* sync and verify incoming physical bitstream */
    ogg_stream_state os; /* take physical pages, weld into a logical
                stream of packets */
    ogg_page         og; /* one Ogg bitstream page. Vorbis packets are inside */
    ogg_packet       op; /* one raw packet of data for decode */

    vorbis_info      vi; /* struct that stores all the static vorbis bitstream
                settings */
    vorbis_comment   vc; /* struct that stores all the bitstream user comments */
    vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
    vorbis_block     vb; /* local working space for packet->PCM decode */

    char *buffer;
    int  bytes;

    Uint8 *decoded = 0;
    int decoded_size = 0;
    int bufsize = 0;

    /********** Decode setup ************/

    ogg_sync_init(&oy); /* Now we can read pages */

    while(1){ /* we repeat if the bitstream is chained */
        int eos=0;
        int i;

        /* grab some data at the head of the stream. We want the first page
           (which is guaranteed to be small and only contain the Vorbis
           stream initial header) We need the first page to get the stream
           serialno. */

        /* submit a 4k block to libvorbis' Ogg layer */
        buffer=ogg_sync_buffer(&oy,4096);
        bytes=(int)SDL_RWread(file,buffer,1,4096);
        ogg_sync_wrote(&oy,bytes);

        /* Get the first page. */
        if(ogg_sync_pageout(&oy,&og)!=1){
            /* have we simply run out of data?  If so, we're done. */
            if(bytes<4096)break;

            /* error case.  Must not be Vorbis data */
            strcpy(errmsg,"Input does not appear to be an Ogg bitstream.\n");
            return 0;
        }

        /* Get the serial number and set up the rest of decode. */
        /* serialno first; use it to set up a logical stream */
        ogg_stream_init(&os,ogg_page_serialno(&og));

        /* extract the initial header from the first page and verify that the
           Ogg bitstream is in fact Vorbis data */

        /* I handle the initial header first instead of just having the code
           read all three Vorbis headers at once because reading the initial
           header is an easy way to identify a Vorbis bitstream and it's
           useful to see that functionality seperated out. */

        vorbis_info_init(&vi);
        vorbis_comment_init(&vc);
        if(ogg_stream_pagein(&os,&og)<0){ 
            /* error; stream version mismatch perhaps */
            strcpy(errmsg,"Error reading first page of Ogg bitstream data.\n");
            return 0;
        }

        if(ogg_stream_packetout(&os,&op)!=1){ 
            /* no page? must not be vorbis */
            strcpy(errmsg,"Error reading initial header packet.\n");
            return 0;
        }

        if(vorbis_synthesis_headerin(&vi,&vc,&op)<0){ 
            /* error case; not a vorbis header */
            strcpy(errmsg,"This Ogg bitstream does not contain Vorbis "
                    "audio data.\n");
            return 0;
        }

        /* At this point, we're sure we're Vorbis. We've set up the logical
           (Ogg) bitstream decoder. Get the comment and codebook headers and
           set up the Vorbis decoder */

        /* The next two packets in order are the comment and codebook headers.
           They're likely large and may span multiple pages. Thus we read
           and submit data until we get our two packets, watching that no
           pages are missing. If a page is missing, error out; losing a
           header page is the only place where missing data is fatal. */

        i=0;
        while(i<2){
            while(i<2){
                int result=ogg_sync_pageout(&oy,&og);
                if(result==0)break; /* Need more data */
                /* Don't complain about missing or corrupt data yet. We'll
                   catch it at the packet output phase */
                if(result==1){
                    ogg_stream_pagein(&os,&og); /* we can ignore any errors here
                                       as they'll also become apparent
                                       at packetout */
                    while(i<2){
                        result=ogg_stream_packetout(&os,&op);
                        if(result==0)break;
                        if(result<0){
                            /* Uh oh; data at some point was corrupted or missing!
                               We can't tolerate that in a header.  Die. */
                            strcpy(errmsg,"Corrupt secondary header.  Exiting.\n");
                            return 0;
                        }
                        result=vorbis_synthesis_headerin(&vi,&vc,&op);
                        if(result<0){
                            strcpy(errmsg,"Corrupt secondary header.  Exiting.\n");
                            return 0;
                        }
                        i++;
                    }
                }
            }
            /* no harm in not checking before adding more */
            buffer=ogg_sync_buffer(&oy,4096);
            bytes=(int)SDL_RWread(file,buffer,1,4096);
            if(bytes==0 && i<2){
                strcpy(errmsg,"End of file before finding all Vorbis headers!\n");
                return 0;
            }
            ogg_sync_wrote(&oy,bytes);
        }

        /* Throw the comments plus a few lines about the bitstream we're
           decoding */
        {
            char **ptr=vc.user_comments;
            while(*ptr){
                //strcpy(errmsg,"%s\n",*ptr);
                ++ptr;
            }
            //strcpy(errmsg,"\nBitstream is %d channel, %ldHz\n",vi.channels,vi.rate);
            //strcpy(errmsg,"Encoded by: %s\n\n",vc.vendor);
        }
    
        spec->freq = (int)vi.rate;
        spec->channels = vi.channels;

        convsize=4096/vi.channels;

        /* OK, got and parsed all three headers. Initialize the Vorbis
           packet->PCM decoder. */
        if(vorbis_synthesis_init(&vd,&vi)==0){ /* central decode state */
            vorbis_block_init(&vd,&vb);          /* local state for most of the decode
                                so multiple block decodes can
                                proceed in parallel. We could init
                                multiple vorbis_block structures
                                for vd here */

            /* The rest is just a straight decode loop until end of stream */
            while(!eos){
                while(!eos){
                    int result=ogg_sync_pageout(&oy,&og);
                    if(result==0)break; /* need more data */
                    if(result<0){ /* missing or corrupt data at this page position */
                        strcpy(errmsg,"Corrupt or missing data in bitstream; "
                                "continuing...\n");
                    }else{
                        ogg_stream_pagein(&os,&og); /* can safely ignore errors at
                                           this point */
                        while(1){
                            result=ogg_stream_packetout(&os,&op);

                            if(result==0)break; /* need more data */
                            if(result<0){ /* missing or corrupt data at this page position */
                                /* no reason to complain; already complained above */
                            }else{
                                /* we have a packet.  Decode it */
                                float **pcm;
                                int samples;

                                if(vorbis_synthesis(&vb,&op)==0) /* test for success! */
                                    vorbis_synthesis_blockin(&vd,&vb);
                                /* 

                                 **pcm is a multichannel float vector.  In stereo, for
                                 example, pcm[0] is left, and pcm[1] is right.  samples is
                                 the size of each channel.  Convert the float values
                                 (-1.<=range<=1.) to whatever PCM format and write it out */

                                while((samples=vorbis_synthesis_pcmout(&vd,&pcm))>0){
                                    int j;
                                    int clipflag=0;
                                    int bout=(samples<convsize?samples:convsize);

                                    /* convert floats to 16 bit signed ints (host order) and
                                       interleave */
                                    for(i=0;i<vi.channels;i++){
                                        ogg_int16_t *ptr=convbuffer+i;
                                        float  *mono=pcm[i];
                                        for(j=0;j<bout;j++){
#if 1
                                            int val=floor(mono[j]*32767.f+.5f);
#else /* optional dither */
                                            int val=mono[j]*32767.f+drand48()-0.5f;
#endif
                                            /* might as well guard against clipping */
                                            if(val>32767){
                                                val=32767;
                                                clipflag=1;
                                            }
                                            if(val<-32768){
                                                val=-32768;
                                                clipflag=1;
                                            }
                                            *ptr=val;
                                            ptr+=vi.channels;
                                        }
                                    }

                                    //if(clipflag)
                                    //strcpy(errmsg,"Clipping in frame %ld\n",(long)(vd.sequence));

                                    int sz = 2*vi.channels*bout;
                                    int bufinc;
                                    if ((sz/4096)*4096 != sz) {
                                        bufinc = ((sz/4096)+1) * 4096;
                                    }
                                    else {
                                        bufinc = sz;
                                    }
                                    if (decoded_size + bufinc > bufsize) {
                                        bufsize += bufinc;
                                        Uint8 *newbuf = new Uint8[bufsize];
                                        if (decoded != 0) {
                                            memcpy(newbuf, decoded, decoded_size);
                                            delete[] decoded;
                                        }
                                        decoded = newbuf;
                                    }
                                    memcpy(decoded+decoded_size, convbuffer, sz);
                                    decoded_size += sz;

                                    vorbis_synthesis_read(&vd,bout); /* tell libvorbis how
                                                        many samples we
                                                        actually consumed */
                                }            
                            }
                        }
                        if(ogg_page_eos(&og))eos=1;
                    }
                }
                if(!eos){
                    buffer=ogg_sync_buffer(&oy,4096);
                    bytes=(int)SDL_RWread(file,buffer,1,4096);
                    ogg_sync_wrote(&oy,bytes);
                    if(bytes==0)eos=1;
                }
            }

            /* ogg_page and ogg_packet structs always point to storage in
               libvorbis.  They're never freed or manipulated directly */

            vorbis_block_clear(&vb);
            vorbis_dsp_clear(&vd);
        }else{
            strcpy(errmsg,"Error: Corrupt header during playback initialization.\n");
        }

        spec->size = decoded_size;

        /* clean up this logical bitstream; before exit we see if we're
           followed by another [chained] */

        ogg_stream_clear(&os);
        vorbis_comment_clear(&vc);
        vorbis_info_clear(&vi);  /* must be called last */

        break; // TMG
    }

    /* OK, clean up the framer */
    ogg_sync_clear(&oy);

    strcpy(errmsg,"Done.\n");

    spec->format = AUDIO_S16; // FIXME!
    spec->silence = 0;
    spec->samples = 4096;
    spec->callback = 0;
    spec->userdata = 0;

    return decoded;
}
#endif

} // End namespace audio

} // End namespace noo

#endif // USE_VORBIS
