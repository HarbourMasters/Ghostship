#ifndef EU_TRANSLATION_H
#define EU_TRANSLATION_H

// EU changes most text to arrays for each language. This define allows these
// differences to be combined.
#ifdef VERSION_EU
    #define LANGUAGE_ARRAY(cmd) cmd[LANGUAGE_FUNCTION]
#else
    #define LANGUAGE_ARRAY(cmd) cmd
#endif

extern_s void *dialog_table_eu_en[];
extern_s void *course_name_table_eu_en[];
extern_s void *act_name_table_eu_en[];

extern_s void *dialog_table_eu_fr[];
extern_s void *course_name_table_eu_fr[];
extern_s void *act_name_table_eu_fr[];

extern_s void *dialog_table_eu_de[];
extern_s void *course_name_table_eu_de[];
extern_s void *act_name_table_eu_de[];

#endif // EU_TRANSLATION_H
