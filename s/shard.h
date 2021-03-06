// shard.h

/*
   A "shard" is a database (replica pair typically) which represents
   one partition of the overall database.
*/

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

#pragma once

#include "../stdafx.h"
#include "../client/dbclient.h"
#include "../client/model.h"
#include "shardkey.h"
#include <boost/utility.hpp>

namespace mongo {

    class DBConfig;
    class ShardManager;
    class ShardObjUnitTest;

    /**
       config.shard
       { ns : "alleyinsider.fs.chunks" , min : {} , max : {} , server : "localhost:30001" }
     */    
    class Shard : public Model , boost::noncopyable {
    public:
        
        BSONObj& getMin(){
            return _min;
        }
        BSONObj& getMax(){
            return _max;
        }

        string getServer(){
            return _server;
        }
        void setServer( string server );

        bool contains( const BSONObj& obj );

        string toString() const;
        operator string() const { return toString(); }

        bool operator==(const Shard& s);

        bool operator!=(const Shard& s){
            return ! ( *this == s );
        }
        
        void getFilter( BSONObjBuilder& b );

        Shard * split();
        Shard * split( const BSONObj& middle );

        virtual const char * getNS(){ return "config.shard"; }
        virtual void serialize(BSONObjBuilder& to);
        virtual void unserialize(const BSONObj& from);
        virtual string modelServer();

    public:
        Shard( ShardManager * info );
        
    private:
        
        void _markModified();

        ShardManager * _manager;
        
        string _ns;
        BSONObj _min;
        BSONObj _max;
        string _server;
        unsigned long long _lastmod;

        bool _modified;
        
        void _split( BSONObj& middle );

        friend class ShardManager;
        friend class ShardObjUnitTest;
    };

    /* config.sharding
         { ns: 'alleyinsider.fs.chunks' , 
           key: { ts : 1 } ,
           shards: [ { min: 1, max: 100, server: a } , { min: 101, max: 200 , server : b } ]
         }
    */
    class ShardManager {
    public:

        ShardManager( DBConfig * config , string ns ,ShardKeyPattern pattern );
        virtual ~ShardManager();

        string getns(){
            return _ns;
        }
        
        int numShards(){ return _shards.size(); }
        bool hasShardKey( const BSONObj& obj );

        Shard& findShard( const BSONObj& obj );
        ShardKeyPattern& getShardKey(){  return _key; }
        
        /**
         * @return number of shards added to the vector
         */
        int getShardsForQuery( vector<Shard*>& shards , const BSONObj& query );

        void save();

        string toString() const;
        operator string() const { return toString(); }
        
    private:
        DBConfig * _config;
        string _ns;
        ShardKeyPattern _key;
        
        vector<Shard*> _shards;
        
        friend class Shard;
    };

} // namespace mongo
