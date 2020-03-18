#pragma once

#include <vector>

#include "BookEntry.h"
#include <map>
#include "BookManagementMessage.h"

#include "Distribution.h"
#include "IntelligentDistribution.h"

class BookLevel : public std::vector<BookEntry*> {
public:
	BookLevel(unsigned int price)
		: m_price(price) { }

	unsigned int price() const { return m_price; }
	unsigned long long volume() const {
		return std::accumulate(this->cbegin(), this->cend(), 0, [](unsigned long long acc, const BookEntry* entry) {
			return acc += entry->quantity;
		});
	}
private:
	unsigned int m_price;
};

class Book {
public:
	Book()
		: m_bestVolumeDistribution(10000), m_lastBestVolumeMeasurementTime(0), m_orderLifetimeDistribution(100), m_volumeLifetimeDistribution(100),
		m_lastSnapshotTime(0), m_orderLifetimeDeltaDistribution(), m_volumeLifetimeDeltaDistribution() { };
	~Book() = default;

	void processMessage(const BookManagementMessage& msg);
	void flushCsv(
		std::ostream& quantitiesStream, std::ostream& countsStream, std::ostream& cancellationQuantitiesStream, std::ostream& cancellationCountsStream, std::ostream& creationCancellationQuantitiesStream, std::ostream& creationCancellationCountsStream,
		std::ostream& countsFractionStream, std::ostream& quantityFractionStream,
		std::ostream& bestVolumeStream, std::ostream& orderLifetimeStream, std::ostream& volumeLifetimeStream,
		std::ostream& orderLifetimeDeltaStream, std::ostream& volumeLifetimeDeltaStream, std::ostream& snapshotMeanStream, std::ostream& snapshotVarianceStream
	) const;
private:
	std::vector<BookLevel> m_bidQueue;
	std::vector<BookLevel> m_askQueue;
	std::map<unsigned long long, BookEntry*> m_entriesById;

	void copyIn(BookEntry* entry, const BookManagementMessage& msg) const;
	void insertIntoQueue(BookEntry* entry);
	void removeFromQueue(const BookEntry* entry);

	Distribution<unsigned long long> m_placementQuantitiesDistribution;
	Distribution<unsigned long long> m_cancellationQuantitiesDistribution;
	Distribution<unsigned long long> m_creationCancellationQuantitiesDistribution;
	Distribution<unsigned long> m_placementCountsDistribution;
	Distribution<unsigned long> m_cancellationCountsDistribution;
	Distribution<unsigned long> m_creationCancellationCountsDistribution;
	IntelligentDistribution<unsigned long long> m_bestVolumeDistribution;
	IntelligentDistribution<unsigned long long> m_orderLifetimeDistribution;
	IntelligentDistribution<unsigned long long> m_volumeLifetimeDistribution;
	Distribution<double> m_volumeLifetimeDeltaDistribution;
	Distribution<unsigned long long> m_orderLifetimeDeltaDistribution;
	Timestamp m_lastBestVolumeMeasurementTime;
	void registerPlacement(BookSide side, unsigned int price, unsigned int volume, Timestamp timestamp);
	void registerCancellation(BookSide side, unsigned int price, unsigned int creationTickOffset, unsigned int volume, Timestamp timestamp, bool isPartial = false);

	using Snapshot = std::vector<unsigned long long>;
	std::vector<Snapshot> m_snapshots; // TODO: check snapshots for overallocating
	void snapshot();
	Timestamp m_lastSnapshotTime;
};