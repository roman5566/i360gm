#!/bin/bash

for i in {1..100}
do
	wget "http://catalog.xboxlive.com/Catalog/Catalog.asmx/Query?methodName=FindGames&Names=Locale&Values=en-US&Names=LegalLocale&Values=en-US&Names=Store&Values=1&Names=PageSize&Values=300&Names=PageNum&Values=$i&Names=DetailView&Values=5&Names=OfferFilterLevel&Values=1&Names=UserTypes&Values=2&Names=MediaTypes&Values=1&Names=MediaTypes&Values=21&Names=MediaTypes&Values=23&Names=MediaTypes&Values=37&Names=MediaTypes&Values=46" -O page$i.xml
done