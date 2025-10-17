#include "database.h"
#include "config.h"

Supabase Database::db;
void Database::init()
{
    db.begin(SUPABASE_URL, SUPABASE_KEY);
}