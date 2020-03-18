#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <set>
#include <map>

#include "BookManagementMessage.h"
#include "Book.h"

#include <ctime>
#include <chrono>

int main(int argc, char** argv) {
	if (argc < 2) {
		return -1;
	}

	std::string fileName = argv[1];
	std::ifstream dataFile(fileName);
	std::istringstream fileNameParts(fileName);
	std::string fileDate;
	std::getline(fileNameParts, fileDate, '_'); std::getline(fileNameParts, fileDate, '_'); std::getline(fileNameParts, fileDate, '_'); std::getline(fileNameParts, fileDate, '_'); std::getline(fileNameParts, fileDate, '-');
	std::cout << "Processing " << fileDate << std::endl;
	std::map<std::string, Book> symbolBooks;

	std::string message;
	while (std::getline(dataFile, message)) {
		std::istringstream messageStream(message);
		BookManagementMessage msg;

		std::string tagPair;
		while (std::getline(messageStream, tagPair, (char)0x01)) {
			std::string tagString, valueString;
			std::size_t equalsPos = tagPair.find('=');
			tagString = tagPair.substr(0, equalsPos);
			valueString = tagPair.substr(equalsPos + 1, std::string::npos);

			unsigned int tag = std::stoul(tagString);
			switch (tag) {
				case 35:
					if (valueString != "X") {
						goto loopend;
					}
					break;
				case 60:
					{
						std::tm timeInfo;
						timeInfo.tm_year = std::stol(valueString.substr(0, 4)) - 1900;
						timeInfo.tm_mon = std::stol(valueString.substr(4, 2))-1;
						timeInfo.tm_mday = std::stol(valueString.substr(6, 2));
						timeInfo.tm_hour = std::stol(valueString.substr(8, 2));
						timeInfo.tm_min = std::stol(valueString.substr(10, 2));
						timeInfo.tm_sec = std::stol(valueString.substr(12, 2));
						unsigned long ns = std::stoul(valueString.substr(14, 9));
						std::time_t timeT = std::mktime(&timeInfo);
						auto timePoint = std::chrono::system_clock::from_time_t(timeT);
						Timestamp timestamp = timePoint.time_since_epoch().count()*100 + ns;
						msg.transactTime = timestamp;
					}
					break;
				case 37708:
				case 279:
					msg.action = (UpdateAction)std::stoul(valueString);
					break;
				case 269:
					if (valueString == "0") {
						msg.entryType = EntryType::Bid;
					} else if (valueString == "1") {
						msg.entryType = EntryType::Offer;
					} else if (valueString == "E") {
						msg.entryType = EntryType::ImpliedBid;
					} else if (valueString == "F") {
						msg.entryType = EntryType::ImpliedOffer;
					}
					break;
				case 48:
					msg.securityId = std::stoul(valueString);
					break;
				case 55:
					msg.symbol = valueString;
					break;
				case 270:
					msg.price = (unsigned int)std::round(std::stod(valueString)*10);
					msg.hasPrice = true;
					break;
				case 271:
					msg.totalQuantity = std::stoul(valueString);
					msg.hasTotalQuantity = true;
					break;
				case 37706:
					msg.displayQuantity = std::stoul(valueString);
					msg.hasDisplayQuantity = true;
					break;
				case 37:
					msg.orderId = std::stoull(valueString);
					break;
				case 37707:
					msg.orderPriority = std::stoull(valueString);
					break;
			}
		}
		{
			auto bookIt = symbolBooks.find(msg.symbol);
			if (bookIt != symbolBooks.end()) {
				if (msg.entryType == EntryType::Bid || msg.entryType == EntryType::Offer) {
					bookIt->second.processMessage(msg);
				}
			} else {
				symbolBooks.insert(bookIt, std::make_pair(msg.symbol, Book()));
			}
		}

	loopend: continue;
	}

	std::ofstream placementQuantitiesFile("placementQuantities_" + fileDate + ".csv");
	std::ofstream placementCountsFile("placementCounts_" + fileDate + ".csv");
	std::ofstream cancellationQuantitiesFile("cancellationQuantities_" + fileDate + ".csv");
	std::ofstream cancellationCountsFile("cancellationCounts_" + fileDate + ".csv");
	std::ofstream creationCancellationQuantitiesFile("creationCancellationQuantities_" + fileDate + ".csv");
	std::ofstream creationCancellationCountsFile("creationCancellationCounts_" + fileDate + ".csv");
	std::ofstream countsFractionFile("countsFractions_" + fileDate + ".csv");
	std::ofstream quantitiesFractionFile("quantitiesFractions_" + fileDate + ".csv");
	std::ofstream bestVolumeFile("bestVolumes_" + fileDate + ".csv");
	std::ofstream orderLifetimeFile("orderLifetimes_" + fileDate + ".csv");
	std::ofstream volumeLifetimeFile("volumeLifetimes_" + fileDate + ".csv");
	std::ofstream orderLifetimeDeltaFile("orderLifetimeDeltas_" + fileDate + ".csv");
	std::ofstream volumeLifetimeDeltaFile("volumeLifetimeDeltas_" + fileDate + ".csv");
	std::ofstream snapshotMeanFile("snapshotMeans_" + fileDate + ".csv");
	std::ofstream snapshotVarianceFile("snapshotVariances_" + fileDate + ".csv");
	for (const auto& symbolBookPair : symbolBooks) {
		placementQuantitiesFile << symbolBookPair.first << ',';
		placementCountsFile << symbolBookPair.first << ',';
		creationCancellationQuantitiesFile << symbolBookPair.first << ',';
		creationCancellationCountsFile << symbolBookPair.first << ',';
		cancellationQuantitiesFile << symbolBookPair.first << ',';
		cancellationCountsFile << symbolBookPair.first << ',';
		countsFractionFile << symbolBookPair.first << ',';
		quantitiesFractionFile << symbolBookPair.first << ',';
		bestVolumeFile << symbolBookPair.first << ',';
		orderLifetimeFile << symbolBookPair.first << ',';
		volumeLifetimeFile << symbolBookPair.first << ',';
		orderLifetimeDeltaFile << symbolBookPair.first << ',';
		volumeLifetimeDeltaFile << symbolBookPair.first << ',';
		snapshotMeanFile << symbolBookPair.first << ',';
		snapshotVarianceFile << symbolBookPair.first << ',';
		
		symbolBookPair.second.flushCsv(
			placementQuantitiesFile, placementCountsFile, cancellationQuantitiesFile, cancellationCountsFile, creationCancellationQuantitiesFile, creationCancellationCountsFile,
			countsFractionFile, quantitiesFractionFile,
			bestVolumeFile, orderLifetimeFile, volumeLifetimeFile, snapshotMeanFile, snapshotVarianceFile,
			orderLifetimeDeltaFile, volumeLifetimeDeltaFile
		);
		
		placementQuantitiesFile << std::endl;
		placementCountsFile << std::endl;
		cancellationQuantitiesFile << std::endl;
		cancellationCountsFile << std::endl;
		creationCancellationQuantitiesFile << std::endl;
		creationCancellationCountsFile << std::endl;
		countsFractionFile << std::endl;
		quantitiesFractionFile << std::endl;
		bestVolumeFile << std::endl;
		orderLifetimeFile << std::endl;
		volumeLifetimeFile << std::endl;
		orderLifetimeDeltaFile << std::endl;
		volumeLifetimeDeltaFile << std::endl;
		snapshotMeanFile << std::endl;
		snapshotVarianceFile << std::endl;
	}
	placementQuantitiesFile.close();
	placementCountsFile.close();

	return 0;
}
