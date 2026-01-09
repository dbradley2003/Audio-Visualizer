#ifndef BAR_H
#define BAR_H


class Bar
{
public:

	Bar(int height, int width, int xLeft,int xRight, int y)
		:
		height_(height),
		width_(width),
		xLeft_(xLeft),
		xRight_(xRight),
		y_(y)
	{
	}

	int height() const
	{
		return height_;
	}

	int width() const
	{
		return width_;
	}

	int xLeft() const
	{
		return xLeft_;
	}

	int xRight() const
	{
		return xRight_;
	}

	int y() const
	{
		return y_;
	}

private:
	int height_;
	int width_;
	int xLeft_;
	int xRight_;
	int y_;
};

#endif