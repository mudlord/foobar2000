#include <vector>
template <typename T>
class circular_buffer
{
	std::vector<T> buffer;
	unsigned readptr, writeptr, used, size;
public:
	circular_buffer() : readptr( 0 ), writeptr( 0 ), size( 0 ), used( 0 ) { buffer.resize(0); }
	void set_size(unsigned p_size){
		reset();
		size = p_size;
		buffer.resize( p_size );
	}
	unsigned data_available() { return used; }
	unsigned free_space() { return size - used; }
	bool write( const T * src, unsigned count )
	{
		if ( count > free_space() ) return false;
		while( count )
		{
			unsigned delta = size - writeptr;
			if ( delta > count ) delta = count;
			std::copy( src, src + delta, buffer.begin() + writeptr );
			used += delta;
			writeptr = ( writeptr + delta ) % size;
			src += delta;
			count -= delta;
		}
		return true;
	}
	unsigned read( T * dst, unsigned count )
	{
		unsigned done = 0;
		for(;;)
		{
			unsigned delta = size - readptr;
			if ( delta > used ) delta = used;
			if ( delta > count ) delta = count;
			if ( !delta ) break;
			std::copy( buffer.begin() + readptr, buffer.begin() + readptr + delta, dst );
			dst += delta;
			done += delta;
			readptr = ( readptr + delta ) % size;
			count -= delta;
			used -= delta;
		}        
		return done;
	}
	void reset()
	{
		readptr = writeptr = used = 0;
	}
};