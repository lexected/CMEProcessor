#pragma once

#include "Timestamp.h"

enum class BookSide {
	BidQueue,
	AskQueue
};

struct BookEntry {
	Timestamp creationTime;
	unsigned long long orderId;
	BookSide side;

	unsigned int price; // multiplied by 10
	int creationDelta;
	unsigned long long bookPriority;
	unsigned long quantity;
};