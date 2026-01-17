#ifndef DRAWABLE_H
#define DRAWABLE_H

#include <array>
#include <memory>
#include <cstdlib>
#include <new>
#include <iostream>

namespace detail
{
	class Concept
	{
	public:
		virtual ~Concept() = default;
		virtual void draw() const = 0;
		virtual void clone(Concept* memory) const = 0;
		virtual void move(Concept* memory) = 0;
	};

	template<typename DrawableT, typename DrawStrategy>
	class OwningModel : public Concept
	{
	public:
		explicit OwningModel(DrawableT drawable, DrawStrategy drawer)
			:drawable_(drawable),
			drawer_(drawer)
		{
		}

		void draw() const override
		{
			drawer_(drawable_);
		}

		void clone(Concept* memory) const override
		{
			auto* ptr =
				const_cast<void*>(static_cast<void const volatile*>(memory));
			::new(ptr) OwningModel(*this);

		}

		void move(Concept* memory) override
		{
			auto* ptr =
				const_cast<void*>(static_cast<void const volatile*>(memory));
			::new(ptr) OwningModel(std::move(*this));
		}

	private:
		DrawableT drawable_;
		DrawStrategy drawer_;
	};
}

using namespace detail;

template<size_t Size =128U, size_t Alignment = sizeof(void*)>
class Drawable
{
public:
	template<typename DrawableT, typename DrawStrategy>
	Drawable(DrawableT drawable, DrawStrategy drawer)
	{
		using Model = OwningModel<DrawableT, DrawStrategy>;
	
		static_assert(sizeof(Model) <= Size, "Given type is too large");
		static_assert(alignof(Model) <= Alignment, "Given type is misaligned");
		
		auto* ptr =
			const_cast<void*>(static_cast<void const volatile*>(pimpl()));
		::new(ptr) Model(std::move(drawable),std::move(drawer));
	}

	Drawable(Drawable const& other)
	{
		other.pimpl()->clone(pimpl());
	}

	Drawable& operator=(Drawable const& other)
	{
		Drawable copy(other);
		storage_.swap(copy.storage_);
		return *this;
	}

	Drawable(Drawable&& other) noexcept
	{
		other.pimpl()->move(pimpl());
	}

	Drawable& operator=(Drawable&& other) noexcept
	{
		Drawable copy(std::move(other));
		storage_.swap(copy.storage_);
		return *this;
	}

	~Drawable()
	{
		std::destroy_at(this->pimpl());
	}

private:
	friend void draw(Drawable const& drawable)
	{
		drawable.pimpl()->draw();
	}

	Concept* pimpl()
	{
		return reinterpret_cast<Concept*>(storage_.data());
	}

	Concept const* pimpl() const
	{
		return reinterpret_cast<Concept const*>(storage_.data());
	}

	alignas(Alignment) std::array<std::byte, Size> storage_;
};

#endif