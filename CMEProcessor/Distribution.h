#pragma once

#include <vector>
#include <numeric>
#include <ostream>
#include <memory>

template <typename T>
class Distribution;

template <typename T>
using DistributionPtr = std::shared_ptr<Distribution<T>>;

template <typename T>
class Distribution {
public:
	Distribution() = default;
	Distribution(const std::vector<T>& data);
	Distribution(std::vector<T>&& data);
	virtual ~Distribution() = default;

	void add(unsigned int distance, T value);
	virtual T& operator[](unsigned int distance);
	virtual const T& operator[](unsigned int distance) const;

	T total() const;

	virtual DistributionPtr<double> normalize() const;
	virtual void flushCsv(std::ostream& stream) const;
protected:
	const T m_default = (T)0;
private:
	std::vector<T> m_data;
};

template<typename T>
inline Distribution<T>::Distribution(const std::vector<T>& data)
	: m_data(data) {
	
}

template<typename T>
inline Distribution<T>::Distribution(std::vector<T>&& data)
	: m_data(data) { }

template<typename T>
inline void Distribution<T>::add(unsigned int distance, T value) {
	this->operator[](distance) += value;
}

template<typename T>
inline T& Distribution<T>::operator[](unsigned int distance) {
	if (distance >= m_data.size()) {
		m_data.resize((size_t)distance + 1, (T)0);
	}

	return m_data[distance];
}

template<typename T>
inline const T& Distribution<T>::operator[](unsigned int distance) const {
	if (distance >= m_data.size()) {
		return m_default;
	}

	return m_data[distance];
}

template<typename T>
inline T Distribution<T>::total() const {
	return std::accumulate(m_data.cbegin(), m_data.cend(), (T)0);
}

template<typename T>
inline DistributionPtr<double> Distribution<T>::normalize() const {
	double total = (double)this->total();

	std::vector<double> newData;
	newData.reserve(m_data.size());
	for (auto it = m_data.cbegin(); it != m_data.cend(); ++it) {
		newData.push_back(*it / total);
	}

	return std::make_shared<Distribution<double>>(newData);
}

#include <string>

template<typename T>
inline void Distribution<T>::flushCsv(std::ostream& stream) const {
	for (const auto& val : m_data) {
		stream << std::to_string(val) << ',';
	}
}
