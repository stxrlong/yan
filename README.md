YAN CPP-ORM (under developing)

- [Overview](#overview)
- [Feature](#feature)
- [Build](#build)
- [Usage](#usage)
  - [SQLITE/MySQL](#sqlitemysql)
  - [MongoDB](#mongodb)
  - [Redis](#redis)
- [Reference](#reference)

**Note**: I'm not a native speaker, if the following instruction is unclear, please feel relaxed to contact the auther '[stxr.long@gmail.com](stxr.long@gmail.com)'

## Overview

Hello, if you are looking for an ORM framework, you can stay for a few minutes to look at this project, this is a completely new design for ORM, supports structured and unstructured databases.

When I first came into using with MySQL/SQLITE, I was confused about why I needed to use such complex SQL statements, and when I started using MongoDB, I was even more confused because its query method was completely different from the SQL language and quite complicated so that I need to learn a new usage. Therefore, in order for subsequent users to quickly use the conventional databases, I designed this project, hoping to greatly simplify the usage of some frequently used databases.

I have analyzed many ORMs on GitHub, and found that they are all based on the condition that users must understand SQL statements, which is not user-friendly. ORMs implemented in languages ​​that support reflection, such as JAVA, also need to process SQL statements, such as the familiar JDBC library 'mybatis'.

For business developers, we know the table structure we designed and how we want to operate the table structure, but we are not familiar with SQL statements or BSON operations, especially when the database name and field name are different from the business code (some mapping operation is required), which makes it easy to make mistakes or bugs. Now this library will helps us to complete business by allowing intuitive operation of table structure, the only thing we have to do is to remember the data tables we created.

## Feature

Thank you for your patient, let me introduce some detailed features (of course, for more information or usage, please refer to the test cases under the directory `/tests/`). Now we support the SQLITE database, MySQL/MongoDB/Redis and they are transaction functions are on the way.

We support the following functions:

- create table
- write
- read (including union/join)
- update
- count
- delete
- copy table value to new table

This project is under developing, greatly wish you can donate some good experience.

## Build

We use the great characteristc `promise/future`, so you can use asynchronous model, we won't introduce the usage of `promise/future` here. 

Because of the omitted function `then` in STL `promise/future`, we depend on the `boost` library to provide the asynchronous feature.

You can directly copy the `src` code to use, or compile and use libyan in `build/lib`. This library already supports common functions, and you can use it according to the supported test cases.

```
mkdir build && cmake -DCONCURRENTQUEUE_PATH=/usr/local/concurrentqueue/ -DSQLITE_PATH=/usr/local/sqlite3/ -DBUILD_TEST=ON .. && ctest
```

## Usage

now we will introduce some simplified usage here, you can dig more from source code

#### SQLITE/MySQL

1. **create tables**

We have a table records the maintain infomation

```
STRUCT(MaintainDB)
MEMBER(MaintainDB, id, YanType::AUTO_INCREMENT)
MEMBER(MaintainDB, name, YanType::STRING)
MEMBER(MaintainDB, type, YanType::INT32)
MEMBER(MaintainDB, point, YanType::INT32)
MEMBER(MaintainDB, data, YanType::INT32)
MEMBER(MaintainDB, online, YanType::DATE)
STRUCT_END
```

We also have another table records the relation-ship between databases and components

```
STRUCT(ComponentDB)
MEMBER(ComponentDB, id, YanType::AUTO_INCREMENT)
MEMBER(ComponentDB, component, YanType::STRING)
MEMBER(ComponentDB, name, YanType::STRING)
MEMBER(ComponentDB, version, YanType::STRING)
MEMBER(ComponentDB, his_version, YanType::STRING)
STRUCT_END

// you can define these tables anywhere as you like
```
Now you can operate the table as struct, suppose you have created a SQLITE object `storage`, and `init` by a determined path, then you can create the same tables in your database like this:

```
auto fut = storage.create_table<MaintainDB>();
auto fut1 = storage.create_table<ComponentDB>();
```

You can also create multiple tables in a same call

```
auto fut = storage.create_table<MaintainDB, ComponentDB>();
```

Don't be worry about the above operations will cover your existed table, it uses the feature `IF NOT EXIST`.

Certainly, we also support three extra features:
1. adding aliases to table names and field names. When you set alias for a table or field, it will not change your operation name, but all the operations under library will use the alias, which may be useful for some situations.
2. adding field length information to those string fields;
3. allowing the field to be null (field cannot be null in default);

You can choose not to set or set any number of these additional features, but when you set multiple additional features, their priority follows the order listed above.

**Attention**: if you want to set a table/field name as a numeric, you must use quote symbol

```
/* set an alias 'mt' for this table */
STRUCT(MaintainDB, mt)
MEMBER(MaintainDB, id, YanType::AUTO_INCREMENT)
/* strlen is less than 64 (the default is 32) */
MEMBER(MaintainDB, name, YanType::STRING, 64) 
/* set an alias 't' for type */
MEMBER(MaintainDB, type, YanType::INT32, t)
/* allow the point field to be null */
MEMBER(MaintainDB, point, YanType::INT32, CAN_BE_NULL)
/* set an alias 'd' for data, and allow it to be null */
MEMBER(MaintainDB, data, YanType::INT32, d, CAN_BE_NULL)
/* you must use quote symbol for an numerical alias */
MEMBER(MaintainDB, online, YanType::DATE, "6")
STRUCT_END
```

2. **write** 

You can write one entity as follows

```
MaintainDB obj;
obj.name = "MySQL";
obj.type = UsageType::STRUCTURED;	// enum UsageType
obj.point = PointType::SERVER;		// enum PointType
obj.data = DataType::MANAGEMENT;	// enum DataType
obj.online = 20220102;

auto fut = storage.write(obj);
```

You can also provider STL sequence container to write many entities at one time

```
std::list<MaintainDB>/std::vector<MaintainDB> objs;
auto fut = storage.write(objs);
```

If you have many object, but you does not want to make a combination by sequence container, you can do

```
auto fut = storage.write(obj1, obj2, obj3);
```

3. **read**

- normal read from one table

Normal read first entity from one table
```
auto fut = storage.read<MaintainDB>();
```

If you want to get all the entities, you can do as follows (of course, you can use the sequence container type as you like)

```
auto fut = storage.read<std::list<MaintainDB>>();
```

You can read by condition (we support multiple condition types), we override the available condition types, you can combine them with combination types including AND/OR/NOT/GROUPBY/OREDER_BY_ASC/ORDER_BY_DESC/... as you want

```
MaintainDB obj;
obj.name.AND() == "MySQL";
obj.online.AND() >= 20220102;
obj.online.AND() < 20220301;
obj.online.ORDER_BY_ASC();
auto fut = storage.read<MaintainDB>(obj);
```

Read all the entities under the condition

```
auto fut = storage.read<std::list<MaintainDB>>(obj);
```

**Delete** and **Count** operations refer to this operation

```
auto fut = storage.count();
auto fut = storage.count(obj);
auto fut = storage.del(); // be careful
auto fut = storage.del(obj);
```

- join read from two or more tables

We just need some fields which is from the above created two structs, we can create a temporary struct like this (**attention**: use prefix `TMP_`):
```
TMP_STRUCT(JoinObj)
/* this third parameter is not YanType, but the STRUCT you defined above */
TMP_MEMBER(JoinObj, id, MaintainDB);
TMP_MEMBER(JoinObj, name, MaintainDB);
TMP_MEMBER(JoinObj, version, ComponentDB);
TMP_STRUCT_END
```

FIELD_BRIDGE is the join condition like `MaintainDB.name = ComponentDB.name`, you must set at lease one join condition when doing `join` read.

```
auto fut = storage.join_read<MaintainDB>()
                  .left_join<ComponentDB>(FIELD_BRIDGE(MaintainDB, name, ComponentDB, name))
                  .commit<JoinObj>();
```

Of course, you can use the sequence container as the return type.

```
auto fut = storage.join_read<MaintainDB>()
                  .left_join<ComponentDB>(FIELD_BRIDGE(MaintainDB, name, ComponentDB, name))
                  .commit<std::list<JoinObj>>();
```

You can also add some condition for this query.

```
MaintainDB cond;
cond.type.AND() == UsageType::NO_SQL;
auto fut = storage.join_read<MaintainDB>()
                  .left_join<ComponentDB>((MaintainDB, name, ComponentDB, name))
                  .commit<std::list<JoinObj>>(cond);
```

You can read values from more tables.

```
auto fut = storage.join_read<A>()
                  .left_join<B>(FIELD_BRIDGE(A, afield, B, bfield))
                  .left_join<C>(FIELD_BRIDGE(A, afield, C, field), 
                                FIELD_BRIDGE(B, bfield1, C, cfield1))
                  .commit<std::list<JoinObj>>(cond);
```

- union read from two or more tables

It is a bit cumbersome to use union read because those tables need to have some similar fields, which requires us to create a temporary table for each table to meet this condition

Suppose we created the following sub-table

```
TMP_STRUCT(SubMaintainDB)
TMP_MEMBER(SubMaintainDB, id, MaintainDB);
TMP_MEMBER(SubMaintainDB, name, MaintainDB);
TMP_STRUCT_END

TMP_STRUCT(SubComponentDB)
TMP_MEMBER(SubComponentDB, id, ComponentDB);
TMP_MEMBER(SubComponentDB, name, ComponentDB);
TMP_STRUCT_END
```

Now we can do union read like this (more than two tables are also supported, you can continue to do `union_read` as long as the structures they return are similar).

```
MaintainDB obj;
obj.point.AND() == PointType::SERVER;
ComponentDB obj1;
obj1.version.AND() == "v5.6";
auto fut = storage.union_read<SubMaintainDB>(obj)
                  .union_read<SubComponentDB>(obj1)
                  .commit<std::list<SubMaintainDB>>();
```

**Attention**: all condition should be created by original tables, those temporary table is only for return type

1. **update**

```
ComponentDB obj;

// set the value you need to update
obj.version = "v5.6";

// set the condition
obj.name.AND() == "MySQL";

auto fut = storage.update(obj);
```

**be careful**: if you do not set the conditon, the whole table will be updated

5. **copy table values to new table**

Suppose you created a new table

```
STRUCT(VersionDB, tv)
MEMBER(VersionDB, id, YanType::AUTO_INCREMENT)
MEMBER(VersionDB, name, YanType::STRING, n)
MEMBER(VersionDB, data, YanType::INT32, d)
STRUCT_END
```

You can do copy like this in SQLITE

```
MaintainDB cond;
cond.type.AND() == UsageType::NO_SQL;
auto fut = storage.copy<MaintainDB>().to<VersionDB>(cond);
```

SQLITE does not support `SELECT INTO`, fortunately, you can copy value from many tables with join operation in MySQL, the JoinObj is defined by yourself which columns are needed to be copied, you cannot omit it because we don't know which columns you need from the joined tables

```
auto fut = storage.copy<MaintainDB>()
                  .left_join<ComponentDB>(FIELD_BRIDGE(MaintainDB, name, ComponentDB, name))
                  .to<VersionDB, JoinObj>(cond);
```

As you see, it's pretty easy when we do some operations on the database, there is no extra codes, and no need to create SQL, all you need to do is to remember the table structures you have created. Unfortunately, we cannot do the above operations using the original library provided by the database itself, but this library can help you to do it.

The entire implementation of this project mainly benefits from the following language features, simply speaking, all above implementions is base on the address operation, which enable us to realize the reflection in CPP:

```
#define __OFFSET__(obj, member) ((size_t)(&((obj*)0)->member))
```
---
The following features are under development and will be released soon. There will be more detailed instructions at that time.

6. **transaction**

This function is developing, but it is easy to implement this function, just like the `join/union` read, we will provider a `Transaction` object from the `storage`， and you can do some operations until you do `commit` or `close`.

```
// create transaction handler
auto &transaction = storage.open_transaction();

// operations
transaction.write(obj);
transaction.delete(cond);

// commit the operations
transaction.commit(); 

// you can also close it, it will rollback the above operations
transaction.close();
```

#### MongoDB

If we only support to create a struct with one level deep, we can not do it for those no-sql databases. The following structure comes from the mongocxx driver's example:

```
{
  "name" : "MongoDB",
  "type" : "database",
  "count" : 1,
  "versions": [ "v6.0", "v5.0", "v4.4", "v4.2", "v4.0", "v3.6" ],
  "drivers" : [{
    "lang" : "cxx",
    "addr" : "https://xxx",
    "downloads" : 123
  }, {
    "lang" : "java",
    "addr" : "https://xxx1",
    "downloads" : 120
  }],
  "info" : {
    "x" : "203",
    "y" : 102
  }
}
```

How can we create such a complicated structure? please continue following my steps:

```
// drivers
STRUCT(Driver)
MEMBER(Driver, lang, YanType::STRING)
MEMBER(Driver, addr, YanType::STRING)
MEMBER(Driver, download, YanType::INT32)
STRUCT_END

// info
STRUCT(Info)
MEMBER(Info, x, YanType::STRING)
MEMBER(Info, y, YanType::INT32)
STRUCT_END

// data
STRUCT(Data)
MEMBER(Data, name, YanType::STRING)
MEMBER(Data, type, YanType::STRING)
MEMBER(Data, count, YanType::INT32)
/* different from the normal member */
LIST_MEMBER(Data, versions, YanType::STRING)
LIST_STRUCT_MEMBER(Data, drivers, Driver)
STRUCT_MEMBER(Data, info, Info)
STRUCT_END
```

If you want to store such a structure, use your familiar way like this:

```
Data data;
data.name = "mongodb";
data.type = "database";
data.count = 1;
data.versions = {"v6.0", "v5.0", "v4.4", "v4.2", "v4.0", "v3.6"};
data.drivers = {
	{
	 .lang = "cxx"; 
	 .addr = "http://xxx";
	 .download = 123
	}, {
	 .lang = "java";
	 .addr = "http://xxx";
	 .download = 120
	}
};
data.info.x = "203";
data.info.y = 102;

auto ret = db.write(data); 
```

If we want to select the values which are satisfied some condtions through the original usage provided by the `mongocxx``, we must do it like this:

```
auto cursor_filtered = collection.find(make_document(
	kvp("name", "mongodb"), 
	kvp(versions, make_document(
		kvp("$eleMatch", make_array("v6.0", "v5.0"))
	))));
```
You have to parse the cursor_filtered if you want to get the detailed values, and what does the conditional symbol `$eleMatch` means here? Why we must use the document condition when doing query for `versions`? As a comparision, How about doing query for the above mongocxx `structure`? :

```
Data cond;
cond.name.AND() == "mongodb";
cond.versions.AND() == std::initializer_list<std::string>{"v6.0", "v5.0"};
auto fut = db.Read<std::list<Data>>(cond);
```

It's OK, does it feel familiar with the recipe and the taste?  [^ _ ^]

#### Redis

Please wait patiently for updates, there will definitely be a surprise

## Reference

Refer to the following orm model: [tiny-orm](https://github.com/david-pp/tiny-orm)