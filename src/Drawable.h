#ifndef DRAWABLE_H
#define DRAWABLE_H

namespace detail
{
	class Concept
	{
	public:
		virtual ~Concept() = default;
		virtual void draw() const = 0;
		virtual std::unique_ptr<Concept> clone() const = 0;
	};
	
	template<typename DrawableT, typename DrawStrategy>
	class OwningModel : public Concept
	{
	public:
		explicit OwningModel(DrawableT drawable, DrawStrategy drawer)
			:drawable_(drawable),
			drawer_(drawer)
		{}

		void draw() const override
		{
			drawer_(drawable_);
		}

		std::unique_ptr<Concept> clone() const override
		{
			return std::make_unique<OwningModel>(*this);
		}

	private:
		DrawableT drawable_;
		DrawStrategy drawer_;
	};
}

class Drawable
{
public:
	template<typename DrawableT, typename DrawStrategy>
	Drawable(DrawableT drawable, DrawStrategy drawer)
	{
		using Model = detail::OwningModel<DrawableT, DrawStrategy>;
		pimpl_ = std::make_unique<Model>(std::move(drawable), std::move(drawer));
	}

	Drawable() = default;

	Drawable(Drawable const& other)
		:pimpl_(other.pimpl_->clone())
	{}

	Drawable& operator=(Drawable const& other)
	{
		Drawable copy(other);
		pimpl_.swap(copy.pimpl_);
		return *this;
	}

	~Drawable() = default;
	Drawable(Drawable&&) = default;
	Drawable& operator=(Drawable&&) = default;

private:
	friend void draw(Drawable const& drawable)
	{
		drawable.pimpl_->draw();
	}

	std::unique_ptr<detail::Concept> pimpl_;
};

#endif