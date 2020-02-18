
trainer: trainer.cpp
	g++ -o trainer trainer.cpp libpugixml.a -lcurl

bayes: bayes.cpp
	g++ -o bayes bayes.cpp
