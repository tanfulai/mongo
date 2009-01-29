
db = connect( "test" );
t = db.getCollection( "basic1" );
assert( t.drop() );

o = { a : 1 };
t.save( o );

assert.eq( 1 , t.findOne().a , "first" );
assert( o._id , "now had id" );
assert( o._id.str , "id not a real id" );

o.a = 2;
t.save( o );

assert.eq( 2 , t.findOne().a , "second" );

assert(t.validate().valid);