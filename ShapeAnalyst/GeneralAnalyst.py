import numpy as np
import pandas as pd
import math

# files to be considered
dates = [
	#"20190901",
	"20190902",
	"20190903",
	"20190904",
	"20190905",
	"20190906",
	#"20190908",
	"20190909",
	"20190910",
	"20190911",
	"20190912",
	"20190913",
	#"20190915",
	"20190916",
	"20190917",
	"20190918",
	"20190919",
	"20190920",
	#"20190922",
	"20190923",
	"20190926",
	"20190927",
	#"20190929",
	"20190930",
]

symbolsOfInterest = [
	"GEH0",
	"GEH1",
	"GEH2",
	"GEH3",
	"GEH4",
	"GEM0",
	"GEM1",
	"GEM2",
	"GEM3",
	"GEM4",
	"GEU0",
	"GEU1",
	"GEU2",
	"GEU3",
	"GEU4",
	"GEZ0",
	"GEZ1",
	"GEZ2",
	"GEZ3",
	"GEZ4"
]

def analyze(inputPrefix, outputPrefix):
	loadedSeries = {}

	for date in dates:
		seriesFile = open(inputPrefix + date + ".csv")
		thisLoadedSeries = pd.DataFrame(
			np.zeros(45, dtype={'names': symbolsOfInterest, 'formats': ['uint64' for col in range(len(symbolsOfInterest))]})
		)
		seriesFileLines = seriesFile.readlines()

		for line in seriesFileLines:
			parts = line.split(',')
			partIterator = iter(parts)
			symbol = next(partIterator)

			if symbolsOfInterest.count(symbol) == 0:
				continue

			tickIndex = 0
			while True:
				try:
					part = next(partIterator)
					if not part.isdigit() or tickIndex >= 45:
						continue

					quantity = int(part)
					thisLoadedSeries.at[tickIndex, symbol] = quantity

					tickIndex += 1
				except StopIteration:
					break

		loadedSeries[date] = thisLoadedSeries

	dayTotals = {}
	for date in dates:
		dayTotals[date] = loadedSeries[date].sum(axis=0)

	meanDayTotals = {}
	accummulatedTotal = {}
	for symbol in symbolsOfInterest:
		total = 0
		for date in dates:
			accummulatedTotal += dayTotals[date]
			total += dayTotals[date][symbol]
		accummulatedTotals[symbol]
		meanDayTotals[symbol] = total // len(dates)

	accummulatedTotal.to_csv(outputPrefix + "_accumulated.csv")

	sdDayTotals = {}
	for symbol in symbolsOfInterest:
		total = 0
		for date in dates:
			total += (dayTotals[date][symbol] - meanDayTotals[symbol]) ** 2.0
		sdDayTotals[symbol] = math.sqrt(total // (len(dates) - 1))

	# print(meanDayTotals)
	# print(sdDayTotals)

	normalisedSeries = {}
	accumulatorNormalisedSeries = pd.DataFrame(
			np.zeros(45, dtype={'names': symbolsOfInterest, 'formats': ['float64' for col in range(len(symbolsOfInterest))]})
	)
	for date in dates:
		normalisedSeries[date] = loadedSeries[date]
		for symbol in symbolsOfInterest:
			normalisedSeries[date][symbol] = normalisedSeries[date][symbol].div(dayTotals[date][symbol])
			# print(normalisedBooks[date][symbol])
		accumulatorNormalisedSeries += normalisedSeries[date]

	shape = accumulatorNormalisedSeries.div(len(dates))

	accumulatorNormalisedVariance = pd.DataFrame(
			np.zeros(45, dtype={'names': symbolsOfInterest, 'formats': ['float64' for col in range(len(symbolsOfInterest))]})
	)
	for date in dates:
		normalisedBookSeries = loadedSeries[date] - shape
		accumulatorNormalisedVariance += normalisedBookSeries**2

	shapeDeviation = accumulatorNormalisedVariance.div(len(dates)) ** 0.5

	shape.to_csv(outputPrefix + "_shape.csv")
	shapeDeviation.to_csv(outputPrefix + "_shapeDeviation.csv")

analyze("D:\\GE_PROCESSED\\snapshotMeans_fut_", "D:\\snapshotMeans")
analyze("D:\\GE_PROCESSED\\cancellationCounts_fut_", "D:\\cancellationCounts")
analyze("D:\\GE_PROCESSED\\cancellationQuantities_fut_", "D:\\cancellationQuantities")
analyze("D:\\GE_PROCESSED\\creationCancellationCounts_fut_", "D:\\creationCancellationCounts")
analyze("D:\\GE_PROCESSED\\creationCancellationQuantities_fut_", "D:\\creationCancellationQuantities")
analyze("D:\\GE_PROCESSED\\placementCounts_fut_", "D:\\placementCounts")
analyze("D:\\GE_PROCESSED\\placementQuantities_fut_", "D:\\placementQuantities")
