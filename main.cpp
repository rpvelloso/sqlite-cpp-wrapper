#include <cstdio>
#include <iostream>

#include "sqlite.c++.h"

int main () {
	
	// DB
	std::string dbName = ":memory:";
	if (dbName != ":memory:")
		remove(dbName.c_str());
	SQLiteDB db(dbName);
	
	// DDL
	SQLiteQuery(db, "create table test ( id number, price bigint, desc varchar );").execute();
	
	// Insert
	SQLiteQuery insert(db, "insert into test values ( ? , ? ,  ? );");
	insert.bind(1, 1);
	insert.bind(2, 998798);
	insert.bind(3, "item 1 description");
	insert.execute();
	
	insert.reset();
	insert.bind(1, 2);
	insert.bind(2, 203948394);
	insert.bind(3, "item 2 description");
	insert.execute();
	
	insert.reset();
	insert.bind(1, 3);
	insert.bind(2, 10293812938);
	insert.bind(3, "item 3 description");
	insert.execute();
	
	// Query
	SQLiteQuery select(db, "select id, price, desc from test where id <> ?;");
	select.bind(1, 2);
	SQLiteResult<int, sqlite3_int64, std::string> result(select);
	
	while (result.next()) {
		struct Record {
			int id;
			sqlite3_int64 price;
			std::string desc;
		} rec;
		
		result.fetch(rec.id, rec.price, rec.desc);
		std::cout << rec.id << ", " << rec.price << ", " << rec.desc << std::endl;
	}
}