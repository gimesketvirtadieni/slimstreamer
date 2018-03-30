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
#include <cstdint>   // std::u..._t types
#include <cstring>   // std::memcpy, std::memcmp, std::memset, std::memchr
#include <stdexcept> // std::out_of_range, std::invalid_argument


namespace slim
{
	namespace util
	{
		class ExpandableBuffer
		{
			public:
				using size_type = std::size_t;
				using byte_type = std::uint8_t;

				inline explicit ExpandableBuffer(size_type s = 0)
				: ExpandableBuffer(s, s) {}

				inline explicit ExpandableBuffer(size_type s, size_type c)
				: size_{s}
				, capacity_{c}
				, free_{true}
				{
					if (s > c)
					{
						throw std::invalid_argument ("size greater than capacity");
					}
					data_ = (c != 0 ? new byte_type[c] : 0);
				}

				inline explicit ExpandableBuffer(const void* d, size_type s)
				: ExpandableBuffer(d, s, s) {}

				inline explicit ExpandableBuffer(const void* d, size_type s, size_type c)
				: size_{s}
				, capacity_{c}
				, free_{true}
				{
					if (s > c)
					{
						throw std::invalid_argument ("size greater than capacity");
					}

					if (c > 0)
					{
						data_ = new byte_type[c];

						if (s > 0)
						{
							std::memcpy(data_, d, s);
						}
					}
					else
					{
						data_ = 0;
					}
				}

				inline explicit ExpandableBuffer(void* d, size_type s, size_type c, bool own)
				: data_{static_cast<byte_type*>(d)}
				, size_{s}
				, capacity_{s}
				, free_{true}
				{
					if (s > c)
					{
						throw std::invalid_argument ("size greater than capacity");
					}
				}

				// TODO: review and refactor
				// there is a need for a custom destructor so Rule Of Zero cannot be used
				// Instead of The Rule of The Big Four (and a half) the following approach is used: http://scottmeyers.blogspot.dk/2014/06/the-drawbacks-of-implementing-move.html
				inline ~ExpandableBuffer()
				{
					if (free_)
					{
						delete[] data_;
					}
				}

				// TODO: move semantic
				inline ExpandableBuffer(const ExpandableBuffer& x)
				: free_{true}
				{
				  if (x.capacity_ != 0)
				  {
					data_ = new byte_type[x.capacity_];

					if (x.size_ != 0)
					  std::memcpy (data_, x.data_, x.size_);
				  }
				  else
					data_ = 0;

				  size_ = x.size_;
				  capacity_ = x.capacity_;
				}

				inline ExpandableBuffer& operator=(const ExpandableBuffer& x)
				{
				  if (&x != this)
				  {
					if (x.size_ > capacity_)
					{
					  if (free_)
						delete[] data_;

					  data_ = new byte_type[x.capacity_];
					  capacity_ = x.capacity_;
					  free_ = true;
					}

					if (x.size_ != 0)
					  std::memcpy (data_, x.data_, x.size_);

					size_ = x.size_;
				  }

				  return *this;
				}

				void swap(ExpandableBuffer&);
				byte_type* detach();

				void assign(const void* data, size_type size);
				void assign(void* data, size_type size, size_type capacity, bool assume_ownership);
				void append(const ExpandableBuffer&);
				void append(const void* data, size_type size);
				void fill(char value = 0);

				inline size_type size() const
				{
					return size_;
				}

				inline bool size(size_type s)
				{
					auto r{false};

					// adjusting capacity if needed
					if(capacity_ < s)
					{
						r = capacity(s);
					}

					// changing the size
					size_ = s;

					return r;
				}

				size_type capacity () const;
				bool capacity (size_type);

				bool isEmpty() const
				{
					return size_ == 0;
				}

				inline void shrinkLeft(size_type pos)
				{
					if (pos >= size_)
					{
						size(0);
					}
					else if (pos > 0)
					{
						size_ -= pos;
						std::memcpy(data_, data_ + pos, size_);
					}
				}

				byte_type* data () const;
				byte_type& operator[] (size_type) const;
				byte_type& at (size_type) const;

			private:
				byte_type* data_;
				size_type  size_;
				size_type  capacity_;
				bool       free_;
		};

		//
		// Implementation.
		//
		inline void ExpandableBuffer::swap(ExpandableBuffer& x)
		{
			byte_type* d (x.data_);
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

		inline ExpandableBuffer::byte_type* ExpandableBuffer::detach ()
		{
			byte_type* r (data_);

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

			data_ = new byte_type[s];
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

		  data_ = static_cast<byte_type*> (d);
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

		  byte_type* d (new byte_type[c]);

		  if (size_ != 0)
			std::memcpy (d, data_, size_);

		  if (free_)
			delete[] data_;

		  data_ = d;
		  capacity_ = c;
		  free_ = true;

		  return true;
		}

		inline ExpandableBuffer::byte_type* ExpandableBuffer::data () const
		{
		  return ExpandableBuffer::data_;
		}

		inline ExpandableBuffer::byte_type& ExpandableBuffer::operator[] (size_type i) const
		{
		  return data_[i];
		}

		inline ExpandableBuffer::byte_type& ExpandableBuffer::at (size_type i) const
		{
		  if (i >= size_)
			throw std::out_of_range ("index out of range");

		  return data_[i];
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
