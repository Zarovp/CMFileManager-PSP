#include <mpg123.h>
#include <stdio.h>
#include <string.h>

#include "audio.h"

static mpg123_handle *mp3;
static u64 frames_read = 0, total_samples = 0;
static long sample_rate = 0;
Audio_Metadata metadata;

// For MP3 ID3 tags
struct genre {
	int code;
	char text[112];
};

// For MP3 ID3 tags
struct genre genreList[] = {
	{0 , "Blues"}, {1 , "Classic Rock"}, {2 , "Country"}, {3 , "Dance"}, {4 , "Disco"}, {5 , "Funk"}, {6 , "Grunge"}, {7 , "Hip-Hop"}, {8 , "Jazz"}, {9 , "Metal"}, {10 , "New Age"},
	{11 , "Oldies"}, {12 , "Other"}, {13 , "Pop"}, {14 , "R&B"}, {15 , "Rap"}, {16 , "Reggae"}, {17 , "Rock"}, {18 , "Techno"}, {19 , "Industrial"}, {20 , "Alternative"},
	{21 , "Ska"}, {22 , "Death Metal"}, {23 , "Pranks"}, {24 , "Soundtrack"}, {25 , "Euro-Techno"}, {26 , "Ambient"}, {27 , "Trip-Hop"}, {28 , "Vocal"}, {29 , "Jazz+Funk"}, {30 , "Fusion"},
	{31 , "Trance"}, {32 , "Classical"}, {33 , "Instrumental"}, {34 , "Acid"}, {35 , "House"}, {36 , "Game"}, {37 , "Sound Clip"}, {38 , "Gospel"}, {39 , "Noise"}, {40 , "Alternative Rock"},
	{41 , "Bass"}, {42 , "Soul"}, {43 , "Punk"}, {44 , "Space"}, {45 , "Meditative"}, {46 , "Instrumental Pop"}, {47 , "Instrumental Rock"}, {48 , "Ethnic"}, {49 , "Gothic"}, {50 , "Darkwave"},
	{51 , "Techno-Industrial"}, {52 , "Electronic"}, {53 , "Pop-Folk"}, {54 , "Eurodance"}, {55 , "Dream"}, {56 , "Southern Rock"}, {57 , "Comedy"}, {58 , "Cult"}, {59 , "Gangsta"}, {60 , "Top 40"},
	{61 , "Christian Rap"}, {62 , "Pop/Funk"}, {63 , "Jungle"}, {64 , "Native US"}, {65 , "Cabaret"}, {66 , "New Wave"}, {67 , "Psychadelic"}, {68 , "Rave"}, {69 , "Showtunes"}, {70 , "Trailer"},
	{71 , "Lo-Fi"}, {72 , "Tribal"}, {73 , "Acid Punk"}, {74 , "Acid Jazz"}, {75 , "Polka"}, {76 , "Retro"}, {77 , "Musical"}, {78 , "Rock & Roll"}, {79 , "Hard Rock"}, {80 , "Folk"},
	{81 , "Folk-Rock"}, {82 , "National Folk"}, {83 , "Swing"}, {84 , "Fast Fusion"}, {85 , "Bebob"}, {86 , "Latin"}, {87 , "Revival"}, {88 , "Celtic"}, {89 , "Bluegrass"}, {90 , "Avantgarde"},
	{91 , "Gothic Rock"}, {92 , "Progressive Rock"}, {93 , "Psychedelic Rock"}, {94 , "Symphonic Rock"}, {95 , "Slow Rock"}, {96 , "Big Band"}, {97 , "Chorus"}, {98 , "Easy Listening"}, {99 , "Acoustic"},
	{100 , "Humour"}, {101 , "Speech"}, {102 , "Chanson"}, {103 , "Opera"}, {104 , "Chamber Music"}, {105 , "Sonata"}, {106 , "Symphony"}, {107 , "Booty Bass"}, {108 , "Primus"}, {109 , "Porn Groove"},
	{110 , "Satire"}, {111 , "Slow Jam"}, {112 , "Club"}, {113 , "Tango"}, {114 , "Samba"}, {115 , "Folklore"}, {116 , "Ballad"}, {117 , "Power Ballad"}, {118 , "Rhytmic Soul"}, {119 , "Freestyle"}, {120 , "Duet"},
	{121 , "Punk Rock"}, {122 , "Drum Solo"}, {123 , "A capella"}, {124 , "Euro-House"}, {125 , "Dance Hall"}, {126 , "Goa"}, {127 , "Drum & Bass"}, {128 , "Club-House"}, {129 , "Hardcore"}, {130 , "Terror"},
	{131 , "Indie"}, {132 , "BritPop"}, {133 , "Negerpunk"}, {134 , "Polsk Punk"}, {135 , "Beat"}, {136 , "Christian Gangsta"}, {137 , "Heavy Metal"}, {138 , "Black Metal"}, {139 , "Crossover"}, {140 , "Contemporary C"},
	{141 , "Christian Rock"}, {142 , "Merengue"}, {143 , "Salsa"}, {144 , "Thrash Metal"}, {145 , "Anime"}, {146 , "JPop"}, {147 , "SynthPop"}
};

// For MP3 ID3 tags
// Helper for v1 printing, get these strings their zero byte.
static void safe_print(char *tag, char *name, char *data, size_t size) {
	char safe[31];
	if (size > 30) 
		return;
	memcpy(safe, data, size);
	safe[size] = 0;
	snprintf(tag, 34, "%s: %s\n", name, safe);
}


// For MP3 ID3 tags
// Print out ID3v1 info.
static void print_v1(Audio_Metadata *ID3tag, mpg123_id3v1 *v1) {
	safe_print(ID3tag->title, "",   v1->title,   sizeof(v1->title));
	safe_print(ID3tag->artist, "",  v1->artist,  sizeof(v1->artist));
	safe_print(ID3tag->album, "",   v1->album,   sizeof(v1->album));
	safe_print(ID3tag->year, "",    v1->year,    sizeof(v1->year));
	safe_print(ID3tag->comment, "", v1->comment, sizeof(v1->comment));
	safe_print(ID3tag->genre, "", genreList[v1->genre].text, sizeof(genreList[v1->genre].text));
}

// For MP3 ID3 tags
// Split up a number of lines separated by \n, \r, both or just zero byte
// and print out each line with specified prefix.
static void print_lines(char *data, const char *prefix, mpg123_string *inlines) {
	size_t i;
	int hadcr = 0, hadlf = 0;
	char *lines = NULL;
	char *line  = NULL;
	size_t len = 0;

	if (inlines != NULL && inlines->fill) {
		lines = inlines->p;
		len   = inlines->fill;
	}
	else 
		return;

	line = lines;
	for (i = 0; i < len; ++i) {
		if (lines[i] == '\n' || lines[i] == '\r' || lines[i] == 0) {
			char save = lines[i]; /* saving, changing, restoring a byte in the data */
			if (save == '\n') 
				++hadlf;
			if (save == '\r') 
				++hadcr;
			if ((hadcr || hadlf) && (hadlf % 2 == 0) && (hadcr % 2 == 0)) 
				line = "";

			if (line) {
				lines[i] = 0;
				if (data == NULL)
					printf("%s%s\n", prefix, line);
				else
					snprintf(data, 0x1F, "%s%s\n", prefix, line);
				line = NULL;
				lines[i] = save;
			}
		}
		else {
			hadlf = hadcr = 0;
			if (line == NULL) 
				line = lines + i;
		}
	}
}

// For MP3 ID3 tags
// Print out the named ID3v2  fields.
static void print_v2(Audio_Metadata *ID3tag, mpg123_id3v2 *v2) {
	print_lines(ID3tag->title, "", v2->title);
	print_lines(ID3tag->artist, "", v2->artist);
	print_lines(ID3tag->album, "", v2->album);
	print_lines(ID3tag->year, "",    v2->year);
	print_lines(ID3tag->comment, "", v2->comment);
	print_lines(ID3tag->genre, "",   v2->genre);
}

int MP3_Init(const char *path) {
	int error = 0, channels = 0;

	error = mpg123_init();
	if (error != MPG123_OK)
		return error;

	mp3 = mpg123_new(NULL, &error);
	if (error != MPG123_OK)
		return error;

	error = mpg123_param(mp3, MPG123_ADD_FLAGS, MPG123_PICTURE, 0.0);
	if (error != MPG123_OK)
		return error;

	error = mpg123_open(mp3, path);
	if (error != MPG123_OK)
		return error;

	static const Audio_Metadata empty;
	metadata = empty;
	mpg123_id3v1 *v1;
	mpg123_id3v2 *v2;

	mpg123_seek(mp3, 0, SEEK_SET);
	metadata.has_meta = mpg123_meta_check(mp3);
	if (metadata.has_meta & MPG123_ID3 && mpg123_id3(mp3, &v1, &v2) == MPG123_OK) {
		if (v1 != NULL)
			print_v1(&metadata, v1);
		if (v2 != NULL) {
			print_v2(&metadata, v2);

			for (size_t count = 0; count < v2->pictures; count++) {
				mpg123_picture *pic = &v2->picture[count];
				char *str = pic->mime_type.p;

				if ((pic->type == 3 ) || (pic->type == 0)) {
					if ((!strcasecmp(str, "image/jpg")) || (!strcasecmp(str, "image/jpeg")) || (!strcasecmp(str, "image/png"))) {
						metadata.cover_image = g2dTexLoadMemory(pic->data, pic->size, G2D_SWIZZLE);
						break;
					}
				}
			}
		}
	}

	total_samples = mpg123_length(mp3);
	mpg123_getformat(mp3, &sample_rate, &channels, NULL);
	mpg123_format_none(mp3);
	mpg123_format(mp3, sample_rate, channels, MPG123_ENC_SIGNED_16);
	return 0;
}

u32 MP3_GetSampleRate(void) {
	return sample_rate;
}

void MP3_Decode(void *buf, unsigned int length, void *userdata) {
	size_t done = 0;
	mpg123_read(mp3, buf, length * (sizeof(s16) * 2), &done);
	frames_read += done/(sizeof(s16) * 2);

	if (frames_read == total_samples)
		playing = false;
}

u64 MP3_GetPosition(void) {
	return frames_read;
}

u64 MP3_GetLength(void) {
	return total_samples;
}

void MP3_Term(void) {
	frames_read = 0;
	
	if (metadata.has_meta) {
		metadata.has_meta = false;

		if (metadata.cover_image)
			g2dTexFree(&metadata.cover_image);
	}

	mpg123_close(mp3);
	mpg123_delete(mp3);
	mpg123_exit();
}
