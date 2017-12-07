# sqlite-cpp-wrapper
Single header SQLite C++ Wrapper (yes, another one, there are many like it, but this one is mine).

# Usage example:
```cpp
#include <cstdio>
#include <iostream>

#include "sqlite.c++.h"

struct Record {
	int id;
	sqlite3_int64 price;
	std::string desc;
};

int main () {
	
	// DB
	std::string dbName = ":memory:";
	if (dbName != ":memory:")
		remove(dbName.c_str());
	SQLiteDB db(dbName);
	
	// DDL
	db.createQuery("create table test ( id number, price bigint, desc varchar );").execute();
	
	// Insert
	Record r = {1, 998798, "item 1 description"};
	auto insert = db.createQuery(
			"insert into test values ( ? , ? ,  ? );",
			r.id, r.price, r.desc); // binded values
	insert.execute();
	
	insert.reset(); // bind one value at a time
	insert.bind(1, 2);
	insert.bind(2, 203948394);
	insert.bind(3, "item 2 description");
	insert.execute();
	
	insert.reset();
	insert.bindValues(3, 10293812938, "item 3 description"); // bind all values
	insert.execute();
	
	// Query
	SQLiteQuery<int> select(db, "select id, price, desc from test where id <> ?;");
	select.bindValues(2);
	SQLiteResult<int, sqlite3_int64, std::string> result(select);
	
	while (result.next()) {
		Record rec;
		
		result.fetch(rec.id, rec.price, rec.desc);
		std::cout << rec.id << ", " << rec.price << ", " << rec.desc << std::endl;
	}
}
```

Output:
```bash
$ ./test.exe
1, 998798, item 1 description
3, 10293812938, item 3 description
```

# Limitations:
- No BLOB support for now...
