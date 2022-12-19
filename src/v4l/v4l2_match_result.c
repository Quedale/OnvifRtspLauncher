
#include <stddef.h>
#include <stdio.h>
#include "v4l2_match_result.h"
#include <gst/gstinfo.h>

GST_DEBUG_CATEGORY_STATIC (ext_v4l2_debug);
#define GST_CAT_DEFAULT (ext_v4l2_debug)

void v4l2MatchResult__init(v4l2MatchResults* self) {
    self->p_match = NULL;
    self->good_matches=malloc(0);
    self->good_matches_count=0;
    self->ok_matches=malloc(0);
    self->ok_matches_count=0;
    self->bad_matches=malloc(0);
    self->bad_matches_count=0;
}

v4l2MatchResults* v4l2MatchResult__create() {
    GST_DEBUG_CATEGORY_INIT (ext_v4l2_debug, "ext-v4l2", 0, "Extension to support v4l capabilities");
    gst_debug_set_threshold_for_name ("ext-v4l2", GST_LEVEL_LOG);
    
    GST_DEBUG("Create result list");
    v4l2MatchResults* result = (v4l2MatchResults*) malloc(sizeof(v4l2MatchResults));
    v4l2MatchResult__init(result);
    return result;
}

void v4l2MatchResult__destroy(v4l2MatchResults* v4l2MatchResult) {
  if (v4l2MatchResult) {
     v4l2MatchResult__clear(v4l2MatchResult, -1);
     free(v4l2MatchResult);
  }
}

v4l2MatchResult ** v4l2MatchResult__remove_element_and_shift(v4l2MatchResults* self, v4l2MatchResult **array, int index, int array_length)
{
    int i;
    for(i = index; i < array_length; i++) {
        array[i] = array[i + 1];
    }
    return array;
}

v4l2MatchResult ** v4l2MatchResult__insert_element_of_type(v4l2MatchResults* self, v4l2MatchResult ** records, v4l2MatchResult * record, int count, int index)
{   
    int i;
    records = realloc (records,sizeof (v4l2MatchResult) * (count+1));
    for(i=count; i> index; i--){
        records[i] = records[i-1];
    }
    records[index]=record;

    return records;
}

void v4l2MatchResult__insert_element(v4l2MatchResults* self, v4l2MatchResult * record, int index)
{   
    MatchTypes type = record->type;
    switch(type){
        case GOOD:
            self->good_matches = v4l2MatchResult__insert_element_of_type(self,self->good_matches,record,self->good_matches_count, index);
            self->good_matches_count++;
            break;
        case OK: 
            self->ok_matches = v4l2MatchResult__insert_element_of_type(self,self->ok_matches,record,self->ok_matches_count, index);
            self->ok_matches_count++;
            break;
        case BAD:
            self->bad_matches = v4l2MatchResult__insert_element_of_type(self,self->bad_matches,record,self->bad_matches_count, index);
            self->bad_matches_count++;
            break;
        case PERFECT:
        default:
            GST_DEBUG("Error : cant insert of this type.");
            break;
    }
};

void v4l2MatchResult__clear(v4l2MatchResults* self, MatchTypes type){
    int i;
    switch(type){
        case GOOD:
            for(i=0; i < self->good_matches_count; i++){
                free(self->good_matches[i]);
            }
            self->good_matches_count = 0;
            self->good_matches = realloc(self->good_matches,0);
            break;
        case OK:
            for(i=0; i < self->ok_matches_count; i++){
                free(self->ok_matches[i]);
            }
            self->ok_matches_count = 0;
            self->ok_matches = realloc(self->ok_matches,0);
            break;
        case BAD:
            for(i=0; i < self->bad_matches_count; i++){
                free(self->bad_matches[i]);
            }
            self->bad_matches_count = 0;
            self->bad_matches = realloc(self->bad_matches,0);
            break;
        case PERFECT:
            free(self->p_match);
            break;
        default:
            v4l2MatchResult__clear(self,PERFECT);
            v4l2MatchResult__clear(self,GOOD);
            v4l2MatchResult__clear(self,OK);
            v4l2MatchResult__clear(self,BAD);
            break;
    };
}

v4l2MatchResult ** v4l2MatchResult__remove_element_by_type(v4l2MatchResults* self, v4l2MatchResult ** records, int count, int index){
    //Remove element and shift content
    records = v4l2MatchResult__remove_element_and_shift(self,records, index, count);  /* First shift the elements, then reallocate */
    //Resize count
    count--;

    //Resize array memory
    records = realloc (records,sizeof(v4l2MatchResult) * count);
    return records;
}

void v4l2MatchResult__remove_element(v4l2MatchResults* self, MatchTypes type, int index){
    switch(type){
        case GOOD:
            self->good_matches = v4l2MatchResult__remove_element_by_type(self,self->good_matches,self->good_matches_count,index);
            break;
        case OK:
            self->ok_matches = v4l2MatchResult__remove_element_by_type(self,self->ok_matches,self->ok_matches_count,index);
            break;
        case BAD:
            self->bad_matches = v4l2MatchResult__remove_element_by_type(self,self->bad_matches,self->bad_matches_count,index);
            break;
        case PERFECT:
        default:
            break;
    };
};