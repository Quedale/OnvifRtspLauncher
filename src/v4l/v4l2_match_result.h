#ifndef V4L2_MATCH_RESULT_H_ 
#define V4L2_MATCH_RESULT_H_

#include <stdlib.h>

typedef enum {
    //TODO Handle fallback where decode/encode would be possible
    //Intentionally not supporting scale up or inserting frame since it would increase bandwidth for no good reason
    PERFECT =             1, //perfect match
    GOOD =                2, //Dropping frames required
    OK =                  3, //Scaling down resolution required
    BAD =                 4, //Drop Frame and Scale down required
    RAW_PERFECT =    5, // Raw capture - perfect match
    RAW_GOOD =       6, // Raw capture - Dropping frames required
    RAW_OK =         7, // Raw capture - Scaling down resolution required
    RAW_BAD =        8  // Raw capture - Drop Frame and Scale down required
} MatchTypes;

typedef struct {
    unsigned int width;
    unsigned int height;
    unsigned int pixel_format;
    unsigned int denominator;
    unsigned int numerator;
    MatchTypes type;
} v4l2MatchResult;

typedef struct {
  int good_matches_count;
  int ok_matches_count;
  int bad_matches_count;
  int raw_good_matches_count;
  int raw_ok_matches_count;
  int raw_bad_matches_count;
  v4l2MatchResult * p_match; //perfect match
  v4l2MatchResult ** good_matches;
  v4l2MatchResult ** ok_matches;
  v4l2MatchResult ** bad_matches;
  v4l2MatchResult * rp_match; //perfect match
  v4l2MatchResult ** raw_good_matches;
  v4l2MatchResult ** raw_ok_matches;
  v4l2MatchResult ** raw_bad_matches;
} v4l2MatchResults;

v4l2MatchResults* v4l2MatchResult__create(); 

void v4l2MatchResult__destroy(v4l2MatchResults* v4l2MatchResult);

void v4l2MatchResult__insert_element(v4l2MatchResults* self, v4l2MatchResult * record, int index);

void v4l2MatchResult__remove_element(v4l2MatchResults* self, MatchTypes type,  int index);
 
void v4l2MatchResult__clear(v4l2MatchResults* self, MatchTypes type);

#endif