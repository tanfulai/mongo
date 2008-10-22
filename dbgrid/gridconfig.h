// gridconfig.h

/**
*    Copyright (C) 2008 10gen Inc.
*  
*    This program is free software: you can redistribute it and/or  modify
*    it under the terms of the GNU Affero General Public License, version 3,
*    as published by the Free Software Foundation.
*  
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU Affero General Public License for more details.
*  
*    You should have received a copy of the GNU Affero General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* This file is things related to the "grid configuration":
   - what machines make up the db component of our cloud
   - where various ranges of things live
*/

#pragma once

#include "../client/dbclient.h"
#include "../client/model.h"

class GridDB {
    DBClientPaired conn;
public:
    enum { Port = 27016 }; /* standard port # for a grid db */
    GridDB();
    void init();
};
extern GridDB gridDB;

/* Machine is the concept of a host that runs the db process.
*/
class Machine { 
public:
    enum { Port = 27018 /* default port # for dbs that are downstream of a dbgrid */
    };
};

typedef map<string,Machine*> ObjLocs;

/* top level grid configuration */
class ClientConfig : public Model { 
};

class GridConfig { 
    ObjLocs loc;

    Machine* fetchOwner(string& client, const char *ns, BSONObj& objOrKey);
public:
    /* return which machine "owns" the object in question -- ie which partition 
       we should go to. 
       
       threadsafe.
    */
    Machine* owner(const char *ns, BSONObj& objOrKey);

    GridConfig();
};

extern GridConfig gridConfig;