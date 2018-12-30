#ifndef __COMMON_NONCOPYABLE_H__
#define __COMMON_NONCOPYABLE_H__

namespace common
{

class noncopyable
{
protected:
	noncopyable() = default;
	~noncopyable() = default;

private:
	noncopyable(const noncopyable& ) = delete;
	const noncopyable& operator=(const noncopyable& ) = delete;
};

} // end namespace common

#endif // __COMMON_NONCOPYABLE_H__
