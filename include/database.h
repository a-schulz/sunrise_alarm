#include <ESPSupabase.h>
#ifndef DATABASE_H
#define DATABASE_H

class Database
{
public:
    static void init();

    static Supabase db;
};

#endif