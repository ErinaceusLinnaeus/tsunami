/*
 * FormatOgg.cpp
 *
 *  Created on: 06.04.2012
 *      Author: michi
 */



#include "FormatOgg.h"
#include "../../Audio/Renderer/AudioRenderer.h"

#ifndef OS_WINDOWS
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <vorbis/vorbisenc.h>


const int CHUNK_SIZE = 1 << 15;



string tag_from_vorbis(const string &key)
{
	if (key == "TRACKNUMBER")
		return "track";
	/*if (key == "DATE")
		return "year";*/
	return key.lower();
}

string tag_to_vorbis(const string &key)
{
	return key.upper();
}


OggVorbis_File vf;
char ogg_buffer[4096];

FormatDescriptorOgg::FormatDescriptorOgg() :
	FormatDescriptor("Ogg vorbis", "ogg", FLAG_AUDIO | FLAG_SINGLE_TRACK | FLAG_TAGS | FLAG_READ | FLAG_WRITE)
{
}



int oe_write_page(ogg_page *page, FILE *fp)
{
    int written;
    written = fwrite(page->header,1,page->header_len, fp);
    written += fwrite(page->body,1,page->body_len, fp);

    return written;
}


void FormatOgg::saveViaRenderer(StorageOperationData *od)
{
	msg_db_f("write_ogg_file", 1);
	AudioRenderer *r = od->renderer;

	float OggQuality = HuiConfig.getFloat("OggQuality", 0.5f);

	FILE *f = fopen(od->filename.c_str(), "wb");

	vorbis_info vi;
	vorbis_info_init(&vi);
	if (vorbis_encode_setup_vbr(&vi, 2, r->getSampleRate(), OggQuality)){
		od->error("vorbis_encode_setup_vbr");
		return;
	}
	vorbis_encode_setup_init(&vi); // ?

	vorbis_dsp_state vd;
	vorbis_block vb;
	ogg_stream_state os;
	if (vorbis_analysis_init(&vd, &vi)){
		od->error("vorbis_analysis_init");
		return;
	}
	if (vorbis_block_init(&vd, &vb)){
		od->error("vorbis_block_init");
		return;
	}
	if (ogg_stream_init(&os, 0)){
		od->error("ogg_stream_init");
		return;
	}

	vorbis_comment vc;
	vorbis_comment_init(&vc);
	Array<Tag> tags = r->getTags();
	for (Tag &tag : tags)
		vorbis_comment_add_tag(&vc, tag.key.c_str(), tag.value.c_str());
	ogg_packet header_main;
	ogg_packet header_comments;
	ogg_packet header_codebooks;
	vorbis_analysis_headerout(&vd, &vc, &header_main, &header_comments, &header_codebooks);
	ogg_stream_packetin(&os, &header_main);
	ogg_stream_packetin(&os,&header_comments);
	ogg_stream_packetin(&os,&header_codebooks);
	ogg_page og;
	ogg_packet op;

	int result;
	while((result = ogg_stream_flush(&os, &og))){
		if (!result)
			break;
		int ret = oe_write_page(&og, f);
		if (ret != og.header_len + og.body_len){
			/*opt->error(_("Failed writing header to output stream\n"));
			ret = 1;
			goto cleanup;*/
			msg_error("ssss");
			return;
		}
	}

//#if 1
	//int eos = 0;
	int written = 0;
	int samples = r->getNumSamples();
#define READSIZE		1<<12
	int nn = 0;

	BufferBox buf;
	buf.resize(READSIZE);

	int eos = 0;
	while(!eos){
		//msg_write(written);

		if (r->readResize(buf) <= 0)
			break;

		float **buffer = vorbis_analysis_buffer(&vd, buf.length);
		for (int i=0;i<buf.length;i++){
			buffer[0][i] = buf.c[0][i];
			buffer[1][i] = buf.c[1][i];
		}
		written += buf.length;
		vorbis_analysis_wrote(&vd, buf.length);


		nn ++;
		if (nn > 8){
			od->set(float(written) / (float)samples);
			nn = 0;
		}

        /* While we can get enough data from the library to analyse, one
           block at a time... */
        while(vorbis_analysis_blockout(&vd,&vb)==1)
        {
				//msg_write("b");

            /* Do the main analysis, creating a packet */
            vorbis_analysis(&vb, NULL);
            vorbis_bitrate_addblock(&vb);

            while (vorbis_bitrate_flushpacket(&vd, &op)){
				//msg_write("f");
                /* Add packet to bitstream */
                ogg_stream_packetin(&os,&op);
                //packetsdone++;

                /* If we've gone over a page boundary, we can do actual output,
                   so do so (for however many pages are available) */

				//eos = 0;
                while( !eos){
				//msg_write("p");
                    int result = ogg_stream_pageout(&os,&og);
                    if (!result)
						break;

                    int ret = oe_write_page(&og, f);
                    if (ret != og.header_len + og.body_len){
                        od->error("Failed writing data to output stream");
                        ret = 1;
                        return;
                    }
                    //else
                      //  bytes_written += ret;

                    if (ogg_page_eos(&og))
                        eos = 1;
                }
            }
        }
	}

	ogg_stream_clear(&os);
	vorbis_block_clear(&vb);
	vorbis_dsp_clear(&vd);
	vorbis_info_clear(&vi);
	fclose(f);
}


void FormatOgg::loadTrack(StorageOperationData *od)
{
	msg_db_f("Ogg.LoadTrack", 1);
	Track *t = od->track;

	if (ov_fopen((char*)od->filename.c_str(), &vf)){
		od->error("ogg: ov_fopen failed");
		return;
	}
	vorbis_info *vi = ov_info(&vf, -1);
	//int bits = 16;
	int channels = 2;
	int freq = DEFAULT_SAMPLE_RATE;
	if (vi){
		channels = vi->channels;
		freq = vi->rate;
	}
	if (t->get_index() == 0)
		t->song->setSampleRate(freq);

	// tags
	t->song->tags.clear();
	char **ptr = ov_comment(&vf,-1)->user_comments;
	while (*ptr){
		string s = *ptr;
		int offset = s.find("=");
		if (offset > 0)
			t->song->tags.add(Tag(tag_from_vorbis(s.head(offset)), s.substr(offset + 1, -1)));
		++ptr;
    }

	int samples = (int)ov_pcm_total(&vf, -1);
	char *data = new char[CHUNK_SIZE];
	int current_section;
	int read = 0;
	int nn = 0;
	while(true){
		int toread = CHUNK_SIZE;
		int r = ov_read(&vf, data, toread, 0, 2, 1, &current_section); // 0,2,1 = little endian, 16bit, signed
		if (r == 0)
			break;
		else if (r < 0){
			od->error("ogg: ov_read failed");
			break;
		}else{
			int dsamples = r / 4;
			int _offset = read / 4 + od->offset;
			importData(t, data, channels, SAMPLE_FORMAT_16, dsamples, _offset, od->level);
			read += r;
			nn ++;
			if (nn > 256){
				od->set((float)read / (float)(samples * 4));
				nn = 0;
			}
		}
	}
	delete[](data);
	ov_clear(&vf);
}

#endif
