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

averageBooks = {}

for date in dates:
	averageBookFile = open("D:\\GE_PROCESSED\\snapshotMeans_fut_" + date + ".csv")
	thisAverageBook = pd.DataFrame(
		np.zeros(45, dtype={'names': symbolsOfInterest, 'formats': ['uint64' for col in range(len(symbolsOfInterest))]})
	)
	averageBookFileLines = averageBookFile.readlines()

	for line in averageBookFileLines:
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
				thisAverageBook.at[tickIndex, symbol] = quantity

				tickIndex += 1
			except StopIteration:
				break

	averageBooks[date] = thisAverageBook

averageVolumes = {}
for date in dates:
	averageVolumes[date] = averageBooks[date].sum(axis=0)

meanVolumes = {}
for symbol in symbolsOfInterest:
	total = 0
	for date in dates:
		total += averageVolumes[date][symbol]
	meanVolumes[symbol] = total // len(symbolsOfInterest)

sdVolumes = {}
for symbol in symbolsOfInterest:
	total = 0
	for date in dates:
		total += (averageVolumes[date][symbol] - meanVolumes[symbol]) ** 2
	sdVolumes[symbol] = math.sqrt(total // (len(symbolsOfInterest) - 1))

print(meanVolumes)
print(sdVolumes)

normalisedBooks = {}
normalisedBookTotal = pd.DataFrame(
		np.zeros(45, dtype={'names': symbolsOfInterest, 'formats': ['float64' for col in range(len(symbolsOfInterest))]})
)
for date in dates:
	normalisedBooks[date] = averageBooks[date]
	for symbol in symbolsOfInterest:
		normalisedBooks[date][symbol] = normalisedBooks[date][symbol].div(averageVolumes[date][symbol])
		# print(normalisedBooks[date][symbol])
	normalisedBookTotal += normalisedBooks[date]

shape = normalisedBookTotal.div(len(dates))

normalisedBookVarianceTotal = pd.DataFrame(
		np.zeros(45, dtype={'names': symbolsOfInterest, 'formats': ['float64' for col in range(len(symbolsOfInterest))]})
)
for date in dates:
	normalisedBookCopy = normalisedBooks[date] - shape
	normalisedBookVarianceTotal += normalisedBookCopy**2

shapeDeviation = normalisedBookVarianceTotal.div(len(dates)) ** 0.5

shape.to_csv("D:\\GE_shape.csv")
shapeDeviation.to_csv("D:\\GE_shapeDeviation.csv")