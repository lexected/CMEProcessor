#include "Book.h"

#include <cmath>

void Book::processMessage(const BookManagementMessage& msg) {
	if (m_lastSnapshotTime + (unsigned long long int)15 * 60 * 1000000000 <= msg.transactTime) {
		m_lastSnapshotTime = msg.transactTime;
		if (m_lastSnapshotTime > 0) {
			snapshot();
		}
	}

	if (msg.action == UpdateAction::New) {
		auto entry = new BookEntry;
		entry->quantity = 0;
		copyIn(entry, msg);
		BookSide side = msg.entryType == EntryType::Bid ? BookSide::BidQueue : BookSide::AskQueue;
		entry->side = side;
		if (side == BookSide::AskQueue) {
			entry->creationDelta = m_askQueue.empty() ? 0 : (int)entry->price - (int)m_askQueue.front().price();
		} else {
			entry->creationDelta = m_bidQueue.empty() ? 0 : (int)m_bidQueue.back().price() - (int)entry->price;
		}

		m_entriesById[msg.orderId] = entry;
		this->registerPlacement(side, msg.price, msg.displayQuantity, msg.transactTime);
		insertIntoQueue(entry);
	} else  if(msg.action == UpdateAction::Update) {
		auto it = m_entriesById.find(msg.orderId);
		if(it != m_entriesById.end()) {
			auto entry = it->second;

			if (entry->quantity < msg.displayQuantity) {
				this->registerPlacement(entry->side, msg.price, msg.displayQuantity - entry->quantity, msg.transactTime);
			} else if (entry->quantity > msg.displayQuantity) {
				Timestamp existentialDuration = msg.transactTime - entry->creationTime;
				if (existentialDuration > 0) {
					unsigned int logDuration = (unsigned int)std::log2(existentialDuration);
					unsigned int deltaQuantity = entry->quantity - msg.displayQuantity;
					m_volumeLifetimeDistribution.add(logDuration, deltaQuantity);

					int relativeCreationTick = entry->creationDelta / 5;
					if (relativeCreationTick >= -10) {
						m_orderLifetimeDeltaDistribution.add(relativeCreationTick + 10, 1);
						m_volumeLifetimeDeltaDistribution.add(relativeCreationTick + 10, ((double)deltaQuantity*1.0e9) / existentialDuration);
						registerCancellation(entry->side, entry->price, relativeCreationTick + 10, deltaQuantity, msg.transactTime, true);
					}
				}
			}

			if (msg.hasPrice && msg.price != entry->price) {
				removeFromQueue(entry);

				int creationRelativeTick = entry->creationDelta / 5;
				if (creationRelativeTick >= -10) {
					registerCancellation(entry->side, entry->price, creationRelativeTick + 10, msg.displayQuantity, msg.transactTime);
				}

				copyIn(entry, msg);
				this->registerPlacement(entry->side, entry->price, entry->quantity, msg.transactTime);
				if (entry->side == BookSide::AskQueue) {
					entry->creationDelta = m_askQueue.empty() ? 0 : (int)entry->price - (int)m_askQueue.front().price();
				} else {
					entry->creationDelta = m_bidQueue.empty() ? 0 : (int)m_bidQueue.back().price() - (int)entry->price;
				}
				insertIntoQueue(entry);
			} else {
				copyIn(entry, msg);
			}
		}
	} else if(msg.action == UpdateAction::Delete) {
		auto it = m_entriesById.find(msg.orderId);
		if (it != m_entriesById.end()) {
			auto entry = it->second;
			removeFromQueue(entry);
			m_entriesById.erase(it);

			Timestamp existentialDuration = msg.transactTime - entry->creationTime;
			if (existentialDuration > 0) {
				unsigned int logDuration = (unsigned int)std::log2(existentialDuration);
				unsigned int deltaQuantity = entry->quantity;
				m_volumeLifetimeDistribution.add(logDuration, deltaQuantity);
				m_orderLifetimeDistribution.add(logDuration, 1);

				int creationRelativeTick = entry->creationDelta / 5;
				if (creationRelativeTick >= -10) {
					m_orderLifetimeDeltaDistribution.add(creationRelativeTick + 10, 1);
					m_volumeLifetimeDeltaDistribution.add(creationRelativeTick + 10, ((double)deltaQuantity * 1.0e9) / existentialDuration);
					unsigned long quantityToDelete;
					if (msg.hasDisplayQuantity) {
						quantityToDelete = msg.displayQuantity;
					} else {
						quantityToDelete = entry->quantity;
					}
					registerCancellation(entry->side, entry->price, creationRelativeTick + 10, quantityToDelete, msg.transactTime);
				}
				
			}

			delete entry;
		}
	}
}

void Book::flushCsv(
	std::ostream& quantitiesStream, std::ostream& countsStream, std::ostream& cancellationQuantitiesStream, std::ostream& cancellationCountsStream, std::ostream& creationCancellationQuantitiesStream, std::ostream& creationCancellationCountsStream,
	std::ostream& countsFractionStream, std::ostream& quantityFractionStream,
	std::ostream& bestVolumeStream, std::ostream& orderLifetimeStream, std::ostream& volumeLifetimeStream,
	std::ostream& orderLifetimeDeltaStream, std::ostream& volumeLifetimeDeltaStream, std::ostream& snapshotMeanStream, std::ostream& snapshotVarianceStream
) const {
	m_placementQuantitiesDistribution.flushCsv(quantitiesStream);
	m_placementCountsDistribution.flushCsv(countsStream);
	m_cancellationQuantitiesDistribution.flushCsv(cancellationQuantitiesStream);
	m_cancellationCountsDistribution.flushCsv(cancellationCountsStream);
	m_creationCancellationQuantitiesDistribution.flushCsv(creationCancellationQuantitiesStream);
	m_creationCancellationCountsDistribution.flushCsv(creationCancellationCountsStream);
	m_bestVolumeDistribution.flushCsv(bestVolumeStream);
	m_orderLifetimeDistribution.flushCsv(orderLifetimeStream);
	m_volumeLifetimeDistribution.flushCsv(volumeLifetimeStream);
	m_orderLifetimeDeltaDistribution.flushCsv(orderLifetimeDeltaStream);
	m_volumeLifetimeDeltaDistribution.flushCsv(volumeLifetimeDeltaStream);

	Distribution<double> quantitiesFractionDistribution;
	quantitiesFractionDistribution[44] = 0.0; // to reserve enough space upfront
	for (unsigned int i = 0; i < 45; ++i) {
		double ratio;
		if (m_placementQuantitiesDistribution[i] == 0.0) {
			ratio = 0.0;
		} else {
			ratio = (double)m_creationCancellationQuantitiesDistribution[i] / m_placementQuantitiesDistribution[i];
		}
		quantitiesFractionDistribution[i] = ratio;
	}
	quantitiesFractionDistribution.flushCsv(quantityFractionStream);

	Distribution<double> countsFractionDistribution;
	countsFractionDistribution[44] = 0.0; // to reserve enough space upfront
	for (unsigned int i = 0; i < 45; ++i) {
		double ratio;
		if (m_placementQuantitiesDistribution[i] == 0.0) {
			ratio = 0.0;
		} else {
			ratio = (double)m_creationCancellationCountsDistribution[i] / m_placementCountsDistribution[i];
		}
		countsFractionDistribution[i] = ratio;
	}
	countsFractionDistribution.flushCsv(countsFractionStream);

	m_placementQuantitiesDistribution.flushCsv(quantitiesStream);
	m_placementCountsDistribution.flushCsv(countsStream);
	m_creationCancellationQuantitiesDistribution.flushCsv(creationCancellationQuantitiesStream);
	m_creationCancellationCountsDistribution.flushCsv(creationCancellationCountsStream);

	Snapshot completeSnapshot;
	size_t maxSnapshotSize = std::accumulate(m_snapshots.begin(), m_snapshots.end(), 0, [](size_t prevMax, const Snapshot& ss) {
		return std::max(prevMax, ss.size());
	});
	const size_t snapshotCount = m_snapshots.size();
	completeSnapshot.resize(maxSnapshotSize, 0);
	for (const Snapshot& snapshot : m_snapshots) {
		for (int i = 0; i < snapshot.size(); ++i) {
			completeSnapshot[i] += snapshot[i];
		}
	}
	for (const auto& val : completeSnapshot) {
		const double meanVolume = (double)val / snapshotCount;
		snapshotMeanStream << meanVolume << ',';
		snapshotVarianceStream << (val - meanVolume) * (val - meanVolume) / (snapshotCount - 1) << ',';
	}
}

void Book::copyIn(BookEntry* entry, const BookManagementMessage& msg) const {
	entry->orderId = msg.orderId;
	if (msg.hasPrice) {
		entry->price = msg.price;
	}
	if (msg.hasDisplayQuantity) {
		entry->quantity = msg.displayQuantity;
	}
	entry->bookPriority = msg.orderPriority;
	entry->creationTime = msg.transactTime;
}

void Book::insertIntoQueue(BookEntry* entry) {
	auto& queue = entry->side == BookSide::BidQueue ? m_bidQueue : m_askQueue;

	auto levelIt = std::lower_bound(queue.begin(), queue.end(), entry->price, [](const BookLevel& lvl, unsigned int desiriedPrice) {
		return lvl.price() < desiriedPrice;
	});
	
	if (levelIt == queue.end() || levelIt->price() != entry->price) {
		auto newIt = queue.insert(levelIt, BookLevel(entry->price));
		newIt->push_back(entry);
	} else {
		auto& level = *levelIt;
		auto orderIt = std::lower_bound(level.begin(), level.end(), entry->bookPriority, [](const BookEntry* e, unsigned long long incomingPriority) {
			return e->bookPriority < incomingPriority;
		});
		level.insert(orderIt, entry);
	}
}

#include <algorithm>

void Book::removeFromQueue(const BookEntry* entry) {
	auto& queue = entry->side == BookSide::BidQueue ? m_bidQueue : m_askQueue;

	auto levelIt = std::lower_bound(queue.begin(), queue.end(), entry->price, [](const BookLevel& lvl, unsigned int desiriedPrice) {
		return lvl.price() < desiriedPrice;
	});

	auto& level = *levelIt;
	std::vector<BookEntry*>::iterator orderIt = std::find(level.begin(), level.end(), entry);
	level.erase(orderIt);

	if (level.empty()) {
		queue.erase(levelIt);
	}
}

void Book::registerPlacement(BookSide side, unsigned int price, unsigned int volume, Timestamp timestamp) {
	int relativePrice;
	if ((side == BookSide::AskQueue && m_askQueue.empty()) || (side == BookSide::BidQueue && m_bidQueue.empty())) {
		relativePrice = 0;
	} else {
		relativePrice = side == BookSide::AskQueue ? (int)price - (int)m_askQueue.front().price() : (int)m_bidQueue.back().price() - (int)price;
	}
	
	int relativeTick = relativePrice / 5;
	if (relativeTick < -10) {
		return;
	}

	unsigned int offsetTick = relativeTick + 10;
	m_placementQuantitiesDistribution.add(offsetTick, volume);
	m_placementCountsDistribution.add(offsetTick, 1);

	if (m_lastBestVolumeMeasurementTime > 0 && !m_askQueue.empty()) {
		Timestamp delta = timestamp - m_lastBestVolumeMeasurementTime;
		m_bestVolumeDistribution.add((unsigned int)m_askQueue.front().volume(), delta);
	}
	m_lastBestVolumeMeasurementTime = timestamp;
}

void Book::registerCancellation(BookSide side, unsigned int price, unsigned int creationTickOffset, unsigned int volume, Timestamp timestamp, bool isPartial) {
	int relativePrice;
	if ((side == BookSide::AskQueue && m_askQueue.empty()) || (side == BookSide::BidQueue && m_bidQueue.empty())) {
		relativePrice = 0;
	} else {
		relativePrice = side == BookSide::AskQueue ? (int)price - (int)m_askQueue.front().price() : (int)m_bidQueue.back().price() - (int)price;
	}

	int relativeTick = relativePrice / 5;
	if (relativeTick < -10) {
		return;
	}

	unsigned int offsetTick = relativeTick + 10;
	m_cancellationQuantitiesDistribution.add(offsetTick, volume);
	m_creationCancellationQuantitiesDistribution.add(creationTickOffset, volume);
	if (!isPartial) { // isPartial==true means that it is not a full order cancellation
		m_cancellationCountsDistribution.add(offsetTick, 1);
		m_creationCancellationCountsDistribution.add(creationTickOffset, 1);
	}
}

void Book::snapshot() {
	if (m_bidQueue.empty() || m_askQueue.empty()) {
		return;
	}

	Snapshot currentSnapshot;
	const int bestBid = m_bidQueue.back().price();
	const int bestAsk = m_askQueue.front().price();
	const int buyPriceDelta = bestBid - m_bidQueue.front().price();
	const int sellPriceDelta = m_askQueue.back().price() - bestAsk;
	currentSnapshot.resize(((size_t)std::max(buyPriceDelta, sellPriceDelta))/5 + 1, 0);

	for (const auto& level : m_bidQueue) {
		const int relativePrice = (bestBid - level.price()) / 5;
		currentSnapshot[relativePrice] += level.volume();
	}

	for (const auto& level : m_askQueue) {
		const int relativePrice = (level.price() - bestAsk) / 5;
		currentSnapshot[relativePrice] += level.volume();
	}

	m_snapshots.push_back(currentSnapshot);
}
