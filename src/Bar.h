#ifndef BAR_H
#define BAR_H

class Bar {
public:
  Bar(const int height, const int width, const int xLeft, const int xRight,
      const int y)
      : height_(height), width_(width), xLeft_(xLeft), xRight_(xRight), y_(y) {}

  [[nodiscard]] int height() const { return height_; }

  [[nodiscard]] int width() const { return width_; }

  [[nodiscard]] int xLeft() const { return xLeft_; }

  [[nodiscard]] int xRight() const { return xRight_; }

  [[nodiscard]] int y() const { return y_; }

private:
  int height_;
  int width_;
  int xLeft_;
  int xRight_;
  int y_;
};

#endif