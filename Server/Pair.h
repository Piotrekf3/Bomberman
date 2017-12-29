#ifndef PAIR_H
#define PAIR_H

class Pair
{
public:
    int x;
    int y;
    Pair(int x, int y)
    {
        this->x = x;
        this->y = y;
    }
	Pair(std::string x, std::string y)
	{
		this->x = stoi(x);
		this->y = stoi(y);
	}
};

#endif
