#pragma once

#include <string>
#include "Timestamp.h"

enum class UpdateAction {
	New = 0,
	Update = 1,
	Delete = 2,

	None
};

enum class EntryType {
	Bid = 0,
	Offer = 1,
	ImpliedBid,
	ImpliedOffer,

	None
};

struct BookManagementMessage {
	Timestamp transactTime;

	int securityId;
	std::string symbol;

	UpdateAction action;
	EntryType entryType;

	unsigned long long orderId;
	unsigned long long int orderPriority;

	bool hasPrice;
	unsigned int price;
	bool hasTotalQuantity;
	unsigned int totalQuantity;
	bool hasDisplayQuantity;
	unsigned int displayQuantity;

	BookManagementMessage()
		: transactTime(), securityId(0), symbol(""), action(UpdateAction::None), entryType(EntryType::None),
		orderId(0), orderPriority(0), hasPrice(false), price(0), hasTotalQuantity(false), totalQuantity(0), hasDisplayQuantity(false), displayQuantity(0) {}

};