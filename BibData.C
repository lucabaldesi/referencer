

#include "bibdata.h"

#include <iostream>

void BibData::print ()
{
	std::cout << "Title: " << title_ << std::endl;	
	std::cout << "Authors: " << authors_ << std::endl;	
	std::cout << "Journal: " << journal_ << std::endl;	
	std::cout << "Volume: " << volume_ << std::endl;	
	std::cout << "Number: " << number_ << std::endl;	
	std::cout << "Pages: " << pages_ << std::endl;	
}
