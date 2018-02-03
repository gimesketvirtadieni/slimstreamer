#pragma once

//
//  Copyright (c) 2011 Boris Kolpackov
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
//  Simple memory buffer abstraction. Version 1.0.0.
//
// https://codesynthesis.com/~boris/blog//2011/08/09/do-we-need-std-buffer/

#pragma once

#include <cstddef>   // std::size_t
#include <cstring>   // std::memcpy, std::memcmp, std::memset, std::memchr
#include <stdexcept> // std::out_of_range, std::invalid_argument


namespace slim
{
	namespace util
	{
		class ExpandableBuffer
		{
			public:
				typedef std::size_t size_type;

				static const size_type npos{static_cast<size_type>(-1)};

				explicit ExpandableBuffer(size_type size = 0);
				explicit ExpandableBuffer(size_type size, size_type capacity);
				explicit ExpandableBuffer(const void* data, size_type size);
				explicit ExpandableBuffer(const void* data, size_type size, size_type capacity);
				explicit ExpandableBuffer(void* data, size_type size, size_type capacity, bool assume_ownership);

			   ~ExpandableBuffer()
				{
					if (free_)
					{
						delete[] data_;
					}
				}

				// TODO: move semantic
				ExpandableBuffer(const ExpandableBuffer&);
				ExpandableBuffer& operator=(const ExpandableBuffer&);

				void swap(ExpandableBuffer&);
				unsigned char* detach();

				void assign(const void* data, size_type size);
				void assign(void* data, size_type size, size_type capacity, bool assume_ownership);
				void append(const ExpandableBuffer&);
				void append(const void* data, size_type size);
				void fill(char value = 0);

				size_type size () const;
				bool size (size_type);
				size_type capacity () const;
				bool capacity (size_type);
				bool empty () const;
				void clear ();

				inline void shrinkLeft(size_type pos)
				{
					if (pos >= size_)
					{
						clear();
					}
					else if (pos > 0)
					{
						size_ -= pos;
						std::memcpy(data_, data_ + pos, size_);
					}
				}

				unsigned char* data ();
				const unsigned char* data () const;

				unsigned char& operator[] (size_type);
				unsigned char operator[] (size_type) const;
				unsigned char& at (size_type);
				unsigned char at (size_type) const;

				size_type find (unsigned char, size_type pos = 0) const;
				size_type rfind (unsigned char, size_type pos = npos) const;

			private:
				unsigned char* data_;
				size_type size_;
				size_type capacity_;
				bool free_;
		};

		bool operator== (const ExpandableBuffer&, const ExpandableBuffer&);
		bool operator!= (const ExpandableBuffer&, const ExpandableBuffer&);

		//
		// Implementation.
		//
		inline ExpandableBuffer::ExpandableBuffer (size_type s)
			: free_ (true)
		{
		  data_ = (s != 0 ? new unsigned char[s] : 0);
		  size_ = capacity_ = s;
		}

		inline ExpandableBuffer::ExpandableBuffer (size_type s, size_type c)
			: free_ (true)
		{
		  if (s > c)
			throw std::invalid_argument ("size greater than capacity");

		  data_ = (c != 0 ? new unsigned char[c] : 0);
		  size_ = s;
		  capacity_ = c;
		}

		inline ExpandableBuffer::ExpandableBuffer (const void* d, size_type s)
			: free_ (true)
		{
		  if (s != 0)
		  {
			data_ = new unsigned char[s];
			std::memcpy (data_, d, s);
		  }
		  else
			data_ = 0;

		  size_ = capacity_ = s;
		}

		inline ExpandableBuffer::ExpandableBuffer (const void* d, size_type s, size_type c)
			: free_ (true)
		{
		  if (s > c)
			throw std::invalid_argument ("size greater than capacity");

		  if (c != 0)
		  {
			data_ = new unsigned char[c];

			if (s != 0)
			  std::memcpy (data_, d, s);
		  }
		  else
			data_ = 0;

		  size_ = s;
		  capacity_ = c;
		}

		inline ExpandableBuffer::ExpandableBuffer (void* d, size_type s, size_type c, bool own)
			: data_ (static_cast<unsigned char*> (d)), size_ (s), capacity_ (c), free_ (own)
		{
		  if (s > c)
			throw std::invalid_argument ("size greater than capacity");
		}

		inline ExpandableBuffer::ExpandableBuffer (const ExpandableBuffer& x)
			: free_ (true)
		{
		  if (x.capacity_ != 0)
		  {
			data_ = new unsigned char[x.capacity_];

			if (x.size_ != 0)
			  std::memcpy (data_, x.data_, x.size_);
		  }
		  else
			data_ = 0;

		  size_ = x.size_;
		  capacity_ = x.capacity_;
		}

		inline ExpandableBuffer& ExpandableBuffer::operator= (const ExpandableBuffer& x)
		{
		  if (&x != this)
		  {
			if (x.size_ > capacity_)
			{
			  if (free_)
				delete[] data_;

			  data_ = new unsigned char[x.capacity_];
			  capacity_ = x.capacity_;
			  free_ = true;
			}

			if (x.size_ != 0)
			  std::memcpy (data_, x.data_, x.size_);

			size_ = x.size_;
		  }

		  return *this;
		}

		inline void ExpandableBuffer::swap (ExpandableBuffer& x)
		{
		  unsigned char* d (x.data_);
		  size_type s (x.size_);
		  size_type c (x.capacity_);
		  bool f (x.free_);

		  x.data_ = data_;
		  x.size_ = size_;
		  x.capacity_ = capacity_;
		  x.free_ = free_;

		  data_ = d;
		  size_ = s;
		  capacity_ = c;
		  free_ = f;
		}

		inline unsigned char* ExpandableBuffer::detach ()
		{
		  unsigned char* r (data_);

		  data_ = 0;
		  size_ = 0;
		  capacity_ = 0;

		  return r;
		}

		inline void ExpandableBuffer::assign (const void* d, size_type s)
		{
		  if (s > capacity_)
		  {
			if (free_)
			  delete[] data_;

			data_ = new unsigned char[s];
			capacity_ = s;
			free_ = true;
		  }

		  if (s != 0)
			std::memcpy (data_, d, s);

		  size_ = s;
		}

		inline void ExpandableBuffer::assign (void* d, size_type s, size_type c, bool own)
		{
		  if (free_)
			delete[] data_;

		  data_ = static_cast<unsigned char*> (d);
		  size_ = s;
		  capacity_ = c;
		  free_ = own;
		}

		inline void ExpandableBuffer::append (const ExpandableBuffer& b)
		{
		  append (b.data (), b.size ());
		}

		inline void ExpandableBuffer::append (const void* d, size_type s)
		{
		  if (s != 0)
		  {
			size_type ns (size_ + s);

			if (capacity_ < ns)
			  capacity (ns);

			std::memcpy (data_ + size_, d, s);
			size_ = ns;
		  }
		}

		inline void ExpandableBuffer::fill (char v)
		{
		  if (size_ > 0)
			std::memset (data_, v, size_);
		}

		inline ExpandableBuffer::size_type ExpandableBuffer::size () const
		{
		  return size_;
		}

		inline bool ExpandableBuffer::size (size_type s)
		{
		  bool r (false);

		  if (capacity_ < s)
			r = capacity (s);

		  size_ = s;
		  return r;
		}

		inline ExpandableBuffer::size_type ExpandableBuffer::capacity () const
		{
		  return capacity_;
		}

		inline bool ExpandableBuffer::capacity (size_type c)
		{
		  // Ignore capacity decrease requests.
		  //
		  if (capacity_ >= c)
			return false;

		  unsigned char* d (new unsigned char[c]);

		  if (size_ != 0)
			std::memcpy (d, data_, size_);

		  if (free_)
			delete[] data_;

		  data_ = d;
		  capacity_ = c;
		  free_ = true;

		  return true;
		}

		inline bool ExpandableBuffer::empty () const
		{
		  return size_ == 0;
		}

		inline void ExpandableBuffer::clear ()
		{
		  size_ = 0;
		}

		inline unsigned char* ExpandableBuffer::data ()
		{
		  return data_;
		}

		inline const unsigned char* ExpandableBuffer::data () const
		{
		  return data_;
		}

		inline unsigned char& ExpandableBuffer::operator[] (size_type i)
		{
		  return data_[i];
		}

		inline unsigned char ExpandableBuffer::operator[] (size_type i) const
		{
		  return data_[i];
		}

		inline unsigned char& ExpandableBuffer::at (size_type i)
		{
		  if (i >= size_)
			throw std::out_of_range ("index out of range");

		  return data_[i];
		}

		inline unsigned char ExpandableBuffer::at (size_type i) const
		{
		  if (i >= size_)
			throw std::out_of_range ("index out of range");

		  return data_[i];
		}

		inline ExpandableBuffer::size_type ExpandableBuffer::find (unsigned char v, size_type pos) const
		{
		  if (size_ == 0 || pos >= size_)
			return npos;

		  unsigned char* p (static_cast<unsigned char*> (std::memchr (data_ + pos, v, size_ - pos)));
		  return p != 0 ? static_cast<size_type> (p - data_) : npos;
		}

		inline ExpandableBuffer::size_type ExpandableBuffer::rfind (unsigned char v, size_type pos) const
		{
		  // memrchr() is not standard.
		  //
		  if (size_ != 0)
		  {
			size_type n (size_);

			if (--n > pos)
			  n = pos;

			for (++n; n-- != 0; )
			  if (data_[n] == v)
				return n;
		  }

		  return npos;
		}

		inline bool operator== (const ExpandableBuffer& a, const ExpandableBuffer& b)
		{
		  return a.size () == b.size () &&
			std::memcmp (a.data (), b.data (), a.size ()) == 0;
		}

		inline bool operator!= (const ExpandableBuffer& a, const ExpandableBuffer& b)
		{
		  return !(a == b);
		}
	}
}
