#pragma once

#include "Distribution.h"
#include <list>
#include <algorithm>
#include <numeric>

template <typename T>
class IntelligentDistribution : public Distribution<T> {
public:
	IntelligentDistribution(unsigned long long learningPointCount);
	IntelligentDistribution(const std::vector<T> & data);
	IntelligentDistribution(std::vector<T> && data);
	virtual ~IntelligentDistribution() = default;

	T& operator[](unsigned int distance) override;
	const T& operator[](unsigned int distance) const override;

	DistributionPtr<double> normalize() const override;
	void flushCsv(std::ostream& stream) const override;

	bool finishedLearning() const { return m_learningPointsLeft == 0; };
private:
	unsigned long long m_learningPointsLeft;
	std::list<std::pair<unsigned long long, T>> m_learningPoints;

	unsigned long long freedmanDiaconisBinWidth() const;
	unsigned long long m_distributionGranularity;
	void prepareGranularStructure();
};

template<typename T>
inline IntelligentDistribution<T>::IntelligentDistribution(unsigned long long learningPointCount)
	: Distribution<T>(), m_learningPointsLeft(learningPointCount), m_distributionGranularity(0) { }

template<typename T>
inline IntelligentDistribution<T>::IntelligentDistribution(const std::vector<T>& data)
	: Distribution<T>(data), m_learningPointsLeft(0), m_distributionGranularity(0) { }

template<typename T>
inline IntelligentDistribution<T>::IntelligentDistribution(std::vector<T>&& data)
	: Distribution<T>(data), m_learningPointsLeft(0), m_distributionGranularity(0) { }

template<typename T>
inline T& IntelligentDistribution<T>::operator[](unsigned int distance) {
	if (m_learningPointsLeft > 0) {
		const auto insertionIt = std::lower_bound(m_learningPoints.begin(), m_learningPoints.end(), distance, [](const auto& pair, unsigned int distance) {
			return pair.first < distance;
		});

		T* ret;
		if (insertionIt != m_learningPoints.end() && insertionIt->first == distance) {
			--m_learningPointsLeft;
			ret = &insertionIt->second;
		} else {
			auto newIt = m_learningPoints.insert(insertionIt, std::make_pair(distance, Distribution<T>::m_default));
			--m_learningPointsLeft;
			ret = &newIt->second;
		}

		if (m_learningPointsLeft == 0) {
			prepareGranularStructure();
		}

		return *ret;
	} else {
		return Distribution<T>::operator[]((unsigned int)((unsigned long long)distance / m_distributionGranularity));
	}
}

template<typename T>
inline const T& IntelligentDistribution<T>::operator[](unsigned int distance) const {
	if (m_learningPointsLeft > 0) {
		const auto lookupIt = std::lower_bound(m_learningPoints.begin(), m_learningPoints.end(), distance, [](const auto& pair, unsigned int distance) {
			return pair.first < distance;
		});
		if (lookupIt != m_learningPoints.end() && lookupIt->first == distance) {
			return lookupIt->second;
		} else {
			return Distribution<T>::m_default;
		}
	} else {
		return Distribution<T>::operator[]((unsigned int)((unsigned long long)distance / m_distributionGranularity));
	}
}

template<typename T>
inline DistributionPtr<double> IntelligentDistribution<T>::normalize() const {
	if (!finishedLearning()) {
		throw std::exception("IntelligentDistribution<T>::normalize(): can not normalize before the distribution completes the learning phase");
	}
	return Distribution<T>::normalize();
}

template<typename T>
inline void IntelligentDistribution<T>::flushCsv(std::ostream& stream) const {
	if (!finishedLearning()) {
		// throw std::exception("IntelligentDistribution<T>::normalize(): can not normalize before the distribution completes the learning phase");
	}

	stream << m_distributionGranularity << ",";
	Distribution<T>::flushCsv(stream);
}

template<typename T>
inline unsigned long long IntelligentDistribution<T>::freedmanDiaconisBinWidth() const {
	auto n = m_learningPoints.size();
	if (n == 0) {
		throw std::exception("IntelligentDistribution<T>::freedmanDiaconisBinWidth(): need a non-empty set of learning points");
	}

	T total = std::accumulate(m_learningPoints.cbegin(), m_learningPoints.cend(), (T)0, [](T acc, const std::pair<unsigned long long, T>& thePair) {
		return acc + thePair.second;
	});
	T lowerBound = total / 4;
	T upperBound = (total / 4) * 3;
	
	auto lbnd = m_learningPoints.cbegin();
	T accumulator = 0;
	for (; lbnd != m_learningPoints.cend(); ++lbnd) {
		accumulator += lbnd->second;
		if (!(accumulator < lowerBound)) {
			break;
		}
	}
	auto ubnd = lbnd;
	for (++ubnd; ubnd != m_learningPoints.cend(); ++ubnd) {
		accumulator += ubnd->second;
		if (!(accumulator < upperBound)) {
			break;
		}
	}
	unsigned long long lqrval = lbnd->first, uqrval = ubnd->first;

	/*unsigned long long lqrn = (unsigned long long)std::floor(0.25 * n);
	unsigned long long uqrn = (unsigned long long)std::ceil(0.75 * n);
	// this is bullshit

	unsigned long long lqrval;
	auto lqrIt = m_learningPoints.cbegin();
	std::advance(lqrIt, lqrn);
	if (lqrIt != m_learningPoints.cend()) {
		lqrval = lqrIt->first;
	} else {
		lqrval = 0;
	}

	unsigned long long uqrval;
	auto uqrIt = m_learningPoints.cbegin();
	std::advance(uqrIt, uqrn);
	if (uqrIt != m_learningPoints.cend()) {
		uqrval = uqrIt->first;
	} else {
		uqrval = m_learningPoints.back().first;
	}*/

	double binWidth = 2 * (uqrval - lqrval) / (double)cbrt(n);
	return (unsigned long long)std::ceil(binWidth);
}

template<typename T>
inline void IntelligentDistribution<T>::prepareGranularStructure() {
	const auto distributionGranularity = freedmanDiaconisBinWidth();
	m_distributionGranularity = distributionGranularity;
	
	for (const auto& pair : m_learningPoints) {
		unsigned int distance = (unsigned int)(pair.first / distributionGranularity);
		Distribution<T>::add(distance, pair.second);
	}

	m_learningPoints.clear();
	m_learningPointsLeft = 0;
}
