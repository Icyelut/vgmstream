#include "meta.h"
#include "../layout/layout.h"
#include "../util.h"

/*
EMFF - Eidos Music File Format (PS2),
found in Tomb Raider Legend/Anniversary, Legacy of Kain - Defiance, possibly more...
*/

VGMSTREAM * init_vgmstream_emff_ps2(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
    int loop_flag = 0;
	int channel_count;
	int frequency;
	int i;
	int j;

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("emff",filename_extension(filename))) goto fail;

    /* do some checks on the file, cause we	have no magic words to check the header...
	it seems if 0x800 and 0x804 = 0 then the file has only audio, if 0x800 = 1
	it has a text section, if both are 1 it's video with a text section included... */

	if (read_32bitBE(0x800,streamFile) == 0x01000000	|| /* "0x01000000" */
		read_32bitBE(0x804,streamFile) == 0x01000000)	/* "0x01000000" */
	goto fail;


    frequency = read_32bitLE(0x0,streamFile);
	channel_count = read_32bitLE(0xC,streamFile);


	if (frequency > 48000 ||
		channel_count > 8) {
		goto fail;
	}

	loop_flag = (read_32bitLE(0x4,streamFile) != 0xFFFFFFFF);
	
	
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */

	start_offset = 0x800;
	vgmstream->sample_rate = frequency;
	vgmstream->channels = channel_count;
	vgmstream->coding_type = coding_PSX;


	if (loop_flag) {
        vgmstream->loop_start_sample = (read_32bitLE(0x28,streamFile)-start_offset)*28/16/channel_count;
        vgmstream->loop_end_sample = read_32bitLE(0x8,streamFile);
    }

    vgmstream->layout_type = layout_emff_ps2_blocked;
    vgmstream->interleave_block_size = 0x10;
    vgmstream->meta_type = meta_EMFF_PS2;

    /* open the file for reading */
    {
        STREAMFILE * file;
        file = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);
        if (!file) goto fail;
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = file;
		}
	}
    
	/* Calc num_samples */
	emff_ps2_block_update(start_offset,vgmstream);
	vgmstream->num_samples=0;

	do {
	
	vgmstream->num_samples += vgmstream->current_block_size*28/16;
		emff_ps2_block_update(vgmstream->next_block_offset,vgmstream);
	} while (vgmstream->next_block_offset<get_streamfile_size(streamFile));

	emff_ps2_block_update(start_offset,vgmstream);


    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}


/*
EMFF - Eidos Music File Format (NGC/WII),
found in Tomb Raider Legend/Anniversary, Legacy of Kain - Defiance, possibly more...
*/

VGMSTREAM * init_vgmstream_emff_ngc(STREAMFILE *streamFile) {
    VGMSTREAM * vgmstream = NULL;
    char filename[260];
    off_t start_offset;
    int loop_flag = 0;
	int channel_count;
	int frequency;
	int i;
	int j;
	off_t coef_table[8] = {0xC8,0xF6,0x124,0x152,0x180,0x1AE,0x1DC,0x20A};

    /* check extension, case insensitive */
    streamFile->get_name(streamFile,filename,sizeof(filename));
    if (strcasecmp("emff",filename_extension(filename))) goto fail;

    /* do some checks on the file, cause we	have no magic words to check the header...
	it seems if 0x800 and 0x804 = 0 then the file has only audio, if 0x800 = 1
	it has a text section, if both are 1 it's video with a text section included... */

	if (read_32bitBE(0x800,streamFile) == 0x00000001 ||	/* "0x00000001" */
		read_32bitBE(0x804,streamFile) == 0x00000001)	/* "0x00000001" */
	goto fail;


    frequency = read_32bitBE(0x0,streamFile);
	channel_count = read_32bitBE(0xC,streamFile);


	if (frequency > 48000 ||
		channel_count > 8) {
		goto fail;
	}

	loop_flag = (read_32bitBE(0x4,streamFile) != 0xFFFFFFFF);
	
	
	/* build the VGMSTREAM */
    vgmstream = allocate_vgmstream(channel_count,loop_flag);
    if (!vgmstream) goto fail;

	/* fill in the vital statistics */

	start_offset = 0x800;
	vgmstream->sample_rate = frequency;
	vgmstream->channels = channel_count;
	vgmstream->coding_type = coding_NGC_DSP;


	if (loop_flag) {
        vgmstream->loop_start_sample = (read_32bitBE(0x28,streamFile)-start_offset)*14/8/channel_count;
        vgmstream->loop_end_sample = read_32bitBE(0x8,streamFile);
    }

    vgmstream->layout_type = layout_emff_ngc_blocked;
    vgmstream->interleave_block_size = 0x10;
    vgmstream->meta_type = meta_EMFF_NGC;

    /* open the file for reading */
    {
        STREAMFILE * file;
        file = streamFile->open(streamFile,filename,STREAMFILE_DEFAULT_BUFFER_SIZE);
        if (!file) goto fail;
        for (i=0;i<channel_count;i++) {
            vgmstream->ch[i].streamfile = file;
		}
	}


	if (vgmstream->coding_type == coding_NGC_DSP) {
			for (j=0;j<vgmstream->channels;j++) {
			for (i=0;i<16;i++) {
		vgmstream->ch[j].adpcm_coef[i] = read_16bitBE(coef_table[j]+i*2,streamFile);
			}
		}
	}

	/* Calc num_samples */
	emff_ngc_block_update(start_offset,vgmstream);
	vgmstream->num_samples=0;

	do {
	
	vgmstream->num_samples += vgmstream->current_block_size*14/8;
		emff_ngc_block_update(vgmstream->next_block_offset,vgmstream);
	} while (vgmstream->next_block_offset<get_streamfile_size(streamFile));

	emff_ngc_block_update(start_offset,vgmstream);


    return vgmstream;

    /* clean up anything we may have opened */
fail:
    if (vgmstream) close_vgmstream(vgmstream);
    return NULL;
}
