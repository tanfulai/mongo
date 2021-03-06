// db.js

if ( typeof DB == "undefined" ){                     
    DB = function( mongo , name ){
        this._mongo = mongo;
        this._name = name;
    }
}

DB.prototype.getMongo = function(){
    assert( this._mongo , "why no mongo!" );
    return this._mongo;
}

DB.prototype.getSisterDB = function( name ){
    return this.getMongo().getDB( name );
}

DB.prototype.getName = function(){
    return this._name;
}

DB.prototype.getCollection = function( name ){
    return new DBCollection( this._mongo , this , name , this._name + "." + name );
}

DB.prototype.commandHelp = function( name ){
    var c = {};
    c[name] = 1;
    c.help = true;
    return this.runCommand( c ).help;
}

DB.prototype.runCommand = function( obj ){
    if ( typeof( obj ) == "string" ){
        var n = {};
        n[obj] = 1;
        obj = n;
    }
    return this.getCollection( "$cmd" ).findOne( obj );
}

DB.prototype._dbCommand = DB.prototype.runCommand;

DB.prototype.addUser = function( username , pass ){
    var c = this.getCollection( "system.users" );
    
    var u = c.findOne( { user : username } ) || { user : username };
    u.pwd = hex_md5( username + ":mongo:" + pass );
    print( tojson( u ) );

    c.save( u );
}

DB.prototype.removeUser = function( username ){
    this.getCollection( "system.users" ).remove( { user : username } );
}

DB.prototype.auth = function( username , pass ){
    var n = this.runCommand( { getnonce : 1 } );

    var a = this.runCommand( 
        { 
            authenticate : 1 , 
            user : username , 
            nonce : n.nonce , 
            key : hex_md5( n.nonce + username + hex_md5( username + ":mongo:" + pass ) )
        }
    );

    return a.ok;
}

/**
  Create a new collection in the database.  Normally, collection creation is automatic.  You would
   use this function if you wish to specify special options on creation.

   If the collection already exists, no action occurs.
   
   <p>Options:</p>
   <ul>
   	<li>
     size: desired initial extent size for the collection.  Must be <= 1000000000.
           for fixed size (capped) collections, this size is the total/max size of the 
           collection.
    </li>
    <li>
     capped: if true, this is a capped collection (where old data rolls out).
    </li>
    <li> max: maximum number of objects if capped (optional).</li>
    </ul>

   <p>Example: </p>
   
   <code>db.createCollection("movies", { size: 10 * 1024 * 1024, capped:true } );</code>
 
 * @param {String} name Name of new collection to create 
 * @param {Object} options Object with options for call.  Options are listed above.
 * @return SOMETHING_FIXME
*/
DB.prototype.createCollection = function(name, opt) {
    var options = opt || {};
    var cmd = { create: name, capped: options.capped, size: options.size, max: options.max };
    var res = this._dbCommand(cmd);
    return res;
}

/**
 *  Returns the current profiling level of this database
 *  @return SOMETHING_FIXME or null on error
 */
 DB.prototype.getProfilingLevel  = function() { 
    var res = this._dbCommand( { profile: -1 } );
    return res ? res.was : null;
}


/**
  Erase the entire database.  (!)
 
 * @return Object returned has member ok set to true if operation succeeds, false otherwise.
*/
DB.prototype.dropDatabase = function() { 	
    return this._dbCommand( { dropDatabase: 1 } );
}


DB.prototype.shutdownServer = function() { 
    if( "admin" != db )
	return "shutdown command only works with the admin database; try 'use admin'";
    return this._dbCommand("shutdown");
}

/**
  Clone database on another server to here.
  <p>
  Generally, you should dropDatabase() first as otherwise the cloned information will MERGE 
  into whatever data is already present in this database.  (That is however a valid way to use 
  clone if you are trying to do something intentionally, such as union three non-overlapping
  databases into one.)
  <p>
  This is a low level administrative function will is not typically used.

 * @param {String} from Where to clone from (dbhostname[:port]).  May not be this database 
                   (self) as you cannot clone to yourself.
 * @return Object returned has member ok set to true if operation succeeds, false otherwise.
 * See also: db.copyDatabase()
*/
DB.prototype.cloneDatabase = function(from) { 
    assert( isString(from) && from.length );
    //this.resetIndexCache();
    return this._dbCommand( { clone: from } );
}


/**
 Clone collection on another server to here.
 <p>
 Generally, you should drop() first as otherwise the cloned information will MERGE 
 into whatever data is already present in this collection.  (That is however a valid way to use 
 clone if you are trying to do something intentionally, such as union three non-overlapping
 collections into one.)
 <p>
 This is a low level administrative function is not typically used.
 
 * @param {String} from mongod instance from which to clnoe (dbhostname:port).  May
 not be this mongod instance, as clone from self is not allowed.
 * @param {String} collection name of collection to clone.
 * @param {Object} query query specifying which elements of collection are to be cloned.
 * @return Object returned has member ok set to true if operation succeeds, false otherwise.
 * See also: db.cloneDatabase()
 */
DB.prototype.cloneCollection = function(from, collection, query) { 
    assert( isString(from) && from.length );
    assert( isString(collection) && collection.length );
    collection = this._name + "." + collection;
    query = query || {};
    //this.resetIndexCache();
    return this._dbCommand( { cloneCollection:collection, from:from, query:query } );
}


/**
  Copy database from one server or name to another server or name.

  Generally, you should dropDatabase() first as otherwise the copied information will MERGE 
  into whatever data is already present in this database (and you will get duplicate objects 
  in collections potentially.)

  For security reasons this function only works when executed on the "admin" db.  However, 
  if you have access to said db, you can copy any database from one place to another.

  This method provides a way to "rename" a database by copying it to a new db name and 
  location.  Additionally, it effectively provides a repair facility.

  * @param {String} fromdb database name from which to copy.
  * @param {String} todb database name to copy to.
  * @param {String} fromhost hostname of the database (and optionally, ":port") from which to 
                    copy the data.  default if unspecified is to copy from self.
  * @return Object returned has member ok set to true if operation succeeds, false otherwise.
  * See also: db.clone()
*/
DB.prototype.copyDatabase = function(fromdb, todb, fromhost) { 
    assert( isString(fromdb) && fromdb.length );
    assert( isString(todb) && todb.length );
    fromhost = fromhost || "";
    //this.resetIndexCache();
    return this._dbCommand( { copydb:1, fromhost:fromhost, fromdb:fromdb, todb:todb } );
}

/**
  Repair database.
 
 * @return Object returned has member ok set to true if operation succeeds, false otherwise.
*/
DB.prototype.repairDatabase = function() {
    return this._dbCommand( { repairDatabase: 1 } );
}


DB.prototype.help = function() {
    print("DB methods:");
    print("\tdb.auth(username, password)");
    print("\tdb.getMongo() get the server connection object");
    print("\tdb.getSisterDB(name) get the db at the same server as this onew");
    print("\tdb.getName()");
    print("\tdb.getCollection(cname) same as db['cname'] or db.cname");
    print("\tdb.runCommand(cmdObj) run a database command.  if cmdObj is a string, turns it into { cmdObj : 1 }");
    print("\tdb.commandHelp(name) returns the help for the command");
    print("\tdb.addUser(username, password)");
    print("\tdb.removeUser(username)");
    print("\tdb.createCollection(name, { size : ..., capped : ..., max : ... } )");
    print("\tdb.getReplicationInfo()");
    print("\tdb.getProfilingLevel()");
    print("\tdb.setProfilingLevel(level) 0=off 1=slow 2=all");
    print("\tdb.cloneDatabase(fromhost)");
    print("\tdb.copyDatabase(fromdb, todb, fromhost)");
    print("\tdb.shutdownServer()");
    print("\tdb.dropDatabase()");
    print("\tdb.repairDatabase()");
    print("\tdb.eval(func, args) run code server-side");
    print("\tdb.getLastError()");
    print("\tdb.getPrevError()");
    print("\tdb.resetError()");
    print("\tdb.getCollectionNames()");
    print("\tdb.group(ns, key[, keyf], cond, reduce, initial)");
}

/**
 * <p> Set profiling level for your db.  Profiling gathers stats on query performance. </p>
 * 
 * <p>Default is off, and resets to off on a database restart -- so if you want it on,
 *    turn it on periodically. </p>
 *  
 *  <p>Levels :</p>
 *   <ul>
 *    <li>0=off</li>
 *    <li>1=log very slow (>100ms) operations</li>
 *    <li>2=log all</li>
 *  @param {String} level Desired level of profiling
 *  @return SOMETHING_FIXME or null on error
 */
DB.prototype.setProfilingLevel = function(level) {
    
    if (level < 0 || level > 2) { 
        throw { dbSetProfilingException : "input level " + level + " is out of range [0..2]" };        
    }
    
    if (level) {
	// if already exists does nothing
		this.createCollection("system.profile", { capped: true, size: 128 * 1024 } );
    }
    return this._dbCommand( { profile: level } );
}


/**
 *  <p> Evaluate a js expression at the database server.</p>
 * 
 * <p>Useful if you need to touch a lot of data lightly; in such a scenario
 *  the network transfer of the data could be a bottleneck.  A good example
 *  is "select count(*)" -- can be done server side via this mechanism.
 * </p>
 *
 * <p>
 * If the eval fails, an exception is thrown of the form:
 * </p>
 * <code>{ dbEvalException: { retval: functionReturnValue, ok: num [, errno: num] [, errmsg: str] } }</code>
 * 
 * <p>Example: </p>
 * <code>print( "mycount: " + db.eval( function(){db.mycoll.find({},{_id:ObjId()}).length();} );</code>
 *
 * @param {Function} jsfunction Javascript function to run on server.  Note this it not a closure, but rather just "code".
 * @return result of your function, or null if error
 * 
 */
DB.prototype.eval = function(jsfunction) {
    var cmd = { $eval : jsfunction };
    if ( arguments.length > 1 ) {
	cmd.args = argumentsToArray( arguments ).slice(1);
    }
    
    var res = this._dbCommand( cmd );
    
    if (!res.ok)
    	throw tojson( res );
    
    return res.retval;
}

DB.prototype.dbEval = DB.prototype.eval;


/**
 * 
 *  <p>
 *   Similar to SQL group by.  For example: </p>
 *
 *  <code>select a,b,sum(c) csum from coll where active=1 group by a,b</code>
 *
 *  <p>
 *    corresponds to the following in 10gen:
 *  </p>
 * 
 *  <code>
     db.group(
       {
         ns: "coll",
         key: { a:true, b:true },
	 // keyf: ...,
	 cond: { active:1 },
	 reduce: function(obj,prev) { prev.csum += obj.c; } ,
	 initial: { csum: 0 }
	 });
	 </code>
 *
 * 
 * <p>
 *  An array of grouped items is returned.  The array must fit in RAM, thus this function is not
 * suitable when the return set is extremely large.
 * </p>
 * <p>
 * To order the grouped data, simply sort it client side upon return.
 * <p>
   Defaults
     cond may be null if you want to run against all rows in the collection
     keyf is a function which takes an object and returns the desired key.  set either key or keyf (not both).
 * </p>
*/
DB.prototype.group = function(parmsObj) {
	
    var groupFunction = function() {
	var parms = args[0];
    	var c = db[parms.ns].find(parms.cond||{});
    	var map = new Map();
        
    	while( c.hasNext() ) {
	    var obj = c.next();
            
	    var key = {};
	    if( parms.key ) {
	    	for( var i in parms.key )
		    key[i] = obj[i];
	    }
	    else {
	    	key = parms.$keyf(obj);
	    }
            
	    var aggObj = map[key];
	    if( aggObj == null ) {
		var newObj = Object.extend({}, key); // clone
	    	aggObj = map[key] = Object.extend(newObj, parms.initial)
	    }
	    parms.$reduce(obj, aggObj);
	}
        
	var ret = map.values();
   	return ret;
    }
    
    var parms = Object.extend({}, parmsObj);
    
    if( parms.reduce ) {
	parms.$reduce = parms.reduce; // must have $ to pass to db
	delete parms.reduce;
    }
    
    if( parms.keyf ) {
	parms.$keyf = parms.keyf;
	delete parms.keyf;
    }
    
    return this.eval(groupFunction, parms);
}

DB.prototype.resetError = function(){
    return this.runCommand( { reseterror : 1 } );
}

DB.prototype.forceError = function(){
    return this.runCommand( { forceerror : 1 } );
}

DB.prototype.getLastError = function(){
    return this.runCommand( { getlasterror : 1 } ).err;
}

/* Return the last error which has occurred, even if not the very last error.

   Returns: 
    { err : <error message>, nPrev : <how_many_ops_back_occurred>, ok : 1 }

   result.err will be null if no error has occurred.
 */
DB.prototype.getPrevError = function(){
    return this.runCommand( { getpreverror : 1 } );
}

DB.prototype.getCollectionNames = function(){
    var all = [];

    var nsLength = this._name.length + 1;
    
    this.getCollection( "system.namespaces" ).find().sort({name:1}).forEach(
        function(z){
            var name = z.name;
            
            if ( name.indexOf( "$" ) >= 0 )
                return;
            
            all.push( name.substring( nsLength ) );
        }
    );
    return all;
}

DB.prototype.tojson = function(){
    return this.toString();
}

DB.prototype.toString = function(){
    return this._name;
}


/** 
  Get a replication log information summary.
  <p>
  This command is for the database/cloud administer and not applicable to most databases.
  It is only used with the local database.  One might invoke from the JS shell:
  <pre>
       use local
       db.getReplicationInfo();
  </pre>
  It is assumed that this database is a replication master -- the information returned is 
  about the operation log stored at local.oplog.$main on the replication master.  (It also 
  works on a machine in a replica pair: for replica pairs, both machines are "masters" from 
  an internal database perspective.
  <p>
  * @return Object timeSpan: time span of the oplog from start to end  if slave is more out 
  *                          of date than that, it can't recover without a complete resync
*/
DB.prototype.getReplicationInfo = function() { 
    if( "local" != this )
	return { errmsg : "this command only works for database local" };

    var result = { };
    var db = this;
    var ol = db.system.namespaces.findOne({name:"local.oplog.$main"});
    if( ol && ol.options ) {
	result.logSizeMB = ol.options.size / 1000 / 1000;
    } else {
	result.errmsg  = "local.oplog.$main, or its options, not found in system.namespaces collection";
	return result;
    }

    var firstc = db.oplog.$main.find().sort({$natural:1}).limit(1);
    var lastc = db.oplog.$main.find().sort({$natural:-1}).limit(1);
    if( !firstc.hasNext() || !lastc.hasNext() ) { 
	result.errmsg = "objects not found in local.oplog.$main -- is this a new and empty db instance?";
	result.oplogMainRowCount = db.oplog.$main.count();
	return result;
    }

    var first = firstc.next();
    var last = lastc.next();
    {
	var tfirst = first.ts;
	var tlast = last.ts;
	if( tfirst && tlast ) { 
	    tfirst = tfirst / 4294967296; // low 32 bits are ordinal #s within a second
	    tlast = tlast / 4294967296;
	    result.timeDiff = tlast - tfirst;
	    result.timeDiffHours = result.timeDiff / 3600;
	    result.tFirst = (new Date(tfirst*1000)).toString();
	    result.tLast  = (new Date(tlast*1000)).toString();
	    result.now = Date();
	}
	else { 
	    result.errmsg = "ts element not found in oplog objects";
	}
    }

    return result;
}
