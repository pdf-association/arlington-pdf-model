#
# Copyright 2021-2022 PDF Association, Inc. https://www.pdfa.org
#
# This material is based upon work supported by the Defense Advanced
# Research Projects Agency (DARPA) under Contract No. HR001119C0079.
# Any opinions, findings and conclusions or recommendations expressed
# in this material are those of the author(s) and do not necessarily
# reflect the views of the Defense Advanced Research Projects Agency
# (DARPA). Approved for public release.
#
# SPDX-License-Identifier: Apache-2.0
# Contributors: Peter Wyatt, PDF Association
#
# A simple makefile to automate some repetitive tasks.
# May require copying dylibs, etc to well known locations on some platforms
#
# First, manually build one of TestGrammar PoC apps (PDFix is fastest):
#  $ make TestGrammar-pdfix     OR
#  $ make TestGrammar-pdfium    OR
#  $ make TestGrammar-qpdf
#
# Then:
#  $ make clean
#  $ make tsv
#  $ make validate
#  $ make 3d
#  $ make xml
#  $ make pandas  <-- Optional, this file is not GitHub!
#
# Since various tasks require Python3 scripts, be sure do the following:
#   python3 -m venv .venv
#   source .venv/bin/activate
#   pip install pikepdf sly
#   python3 ./scripts/... ...
#   deactivate

XMLLINT ::= xmllint
XMLLINT_FLAGS ::= --noout

# Clean up all outputs that can be re-created
.PHONY: clean
clean:
	rm -rf ./3dvisualize/*.json ./xml/*.xml ./scripts/*.tsv
	rm -rf ./tsv/1.?/*.tsv ./tsv/2.0/*.tsv
	rm -rf ./gcxml/dist/gcxml.jar
	rm -rf /TestGrammar/doc


# Make the monolithic TSV file by combining all TSVs - suitable for Jupyter
.PHONY: pandas
pandas:
	python3 ./scripts/arlington-to-pandas.py --tsvdir ./tsv/latest --save ./scripts/pandas.tsv


# Make the 3D/VR visualization JSON files (all versions)
# Ensure to do a "make tsv" beforehand to refresh the PDF version specific file sets!

.PHONY: 3d
3d:
	python3 ./3dvisualize/TSVto3D.py --tsvdir ./tsv/latest --outdir ./3dvisualize/
	python3 ./3dvisualize/TSVto3D.py --tsvdir ./tsv/2.0 --outdir ./3dvisualize/
	python3 ./3dvisualize/TSVto3D.py --tsvdir ./tsv/1.7 --outdir ./3dvisualize/
	python3 ./3dvisualize/TSVto3D.py --tsvdir ./tsv/1.6 --outdir ./3dvisualize/
	python3 ./3dvisualize/TSVto3D.py --tsvdir ./tsv/1.5 --outdir ./3dvisualize/
	python3 ./3dvisualize/TSVto3D.py --tsvdir ./tsv/1.4 --outdir ./3dvisualize/
	python3 ./3dvisualize/TSVto3D.py --tsvdir ./tsv/1.3 --outdir ./3dvisualize/
	python3 ./3dvisualize/TSVto3D.py --tsvdir ./tsv/1.2 --outdir ./3dvisualize/
	python3 ./3dvisualize/TSVto3D.py --tsvdir ./tsv/1.1 --outdir ./3dvisualize/
	python3 ./3dvisualize/TSVto3D.py --tsvdir ./tsv/1.0 --outdir ./3dvisualize/


# Build the TestGrammar C++ PoC app using PDFix (because build times are much faster)
.PHONY: TestGrammar-pdfix
TestGrammar-pdfix:
	rm -rf ./TestGrammar/bin/linux/TestGrammar ./TestGrammar/bin/linux/TestGrammar_d
	rm -rf ./TestGrammar/cmake-linux
	cmake -B ./TestGrammar/cmake-linux/debug -DPDFSDK_PDFIX=ON -DCMAKE_BUILD_TYPE=Debug ./TestGrammar
	cmake --build ./TestGrammar/cmake-linux/debug --config Debug
	cmake -B ./TestGrammar/cmake-linux/release -DPDFSDK_PDFIX=ON -DCMAKE_BUILD_TYPE=Release ./TestGrammar
	cmake --build ./TestGrammar/cmake-linux/release --config Release
	rm -rf ./TestGrammar/cmake-linux


# Build the TestGrammar C++ PoC app using PDFIUM (SLOW!)
.PHONY: TestGrammar-pdfium
TestGrammar-pdfium:
	rm -rf ./TestGrammar/bin/linux/TestGrammar ./TestGrammar/bin/linux/TestGrammar_d
	rm -rf ./TestGrammar/cmake-linux
	cmake -B ./TestGrammar/cmake-linux/debug -DPDFSDK_PDFIUM=ON -DCMAKE_BUILD_TYPE=Debug ./TestGrammar
	cmake --build ./TestGrammar/cmake-linux/debug --config Debug
	cmake -B ./TestGrammar/cmake-linux/release -DPDFSDK_PDFIUM=ON -DCMAKE_BUILD_TYPE=Release ./TestGrammar
	cmake --build ./TestGrammar/cmake-linux/release --config Release
	rm -rf ./TestGrammar/cmake-linux


# Build the TestGrammar C++ PoC app using QPDF (not functional yet!)
.PHONY: TestGrammar-qpdf
TestGrammar-qpdf:
	rm -rf ./TestGrammar/bin/linux/TestGrammar ./TestGrammar/bin/linux/TestGrammar_d
	rm -rf ./TestGrammar/cmake-linux
	cmake -B ./TestGrammar/cmake-linux/debug -DPDFSDK_QPDF=ON -DCMAKE_BUILD_TYPE=Debug ./TestGrammar
	cmake --build ./TestGrammar/cmake-linux/debug --config Debug
	cmake -B ./TestGrammar/cmake-linux/release -DPDFSDK_QPDF=ON -DCMAKE_BUILD_TYPE=Release ./TestGrammar
	cmake --build ./TestGrammar/cmake-linux/release --config Release
	rm -rf ./TestGrammar/cmake-linux


# Validate each of the existing TSV file sets using both the Python script and C++ PoC.
# Does NOT create the TSVs!
# Ensure to do a "make tsv" beforehand to refresh the PDF version specific file sets!
validate:
# Clean-up where gcxml is missing some capabilities...
	rm -f ./tsv/1.3/ActionNOP.tsv ./tsv/1.3/ActionSetState.tsv
	rm -f ./tsv/1.4/ActionNOP.tsv ./tsv/1.4/ActionSetState.tsv
	rm -f ./tsv/1.5/ActionNOP.tsv ./tsv/1.5/ActionSetState.tsv
	rm -f ./tsv/1.6/ActionNOP.tsv ./tsv/1.6/ActionSetState.tsv
	rm -f ./tsv/1.7/ActionNOP.tsv ./tsv/1.7/ActionSetState.tsv
	rm -f ./tsv/2.0/ActionNOP.tsv ./tsv/2.0/ActionSetState.tsv

	mv  ./tsv/1.4/XObjectImage.tsv ./tsv/1.4/XObjectImage-BEFORE.tsv
	sed -E 's/\[fn\:Not\(fn\:IsPresent\(\@SMaskInData>0\)\)\]//g' ./tsv/1.4/XObjectImage-BEFORE.tsv > ./tsv/1.4/XObjectImage.tsv
	rm ./tsv/1.4/XObjectImage-BEFORE.tsv

	mv ./tsv/1.3/AnnotStamp.tsv ./tsv/1.3/AnnotStamp-BEFORE.tsv
	sed -E 's/\[fn\:Not\(fn\:IsRequired\(fn\:IsPresent\(IT\) && \(\@IT!=Stamp\)\)\)\]//g' ./tsv/1.3/AnnotStamp-BEFORE.tsv > ./tsv/1.3/AnnotStamp.tsv
	rm ./tsv/1.3/AnnotStamp-BEFORE.tsv

	mv ./tsv/1.4/AnnotStamp.tsv ./tsv/1.4/AnnotStamp-BEFORE.tsv
	sed -E 's/\[fn\:Not\(fn\:IsRequired\(fn\:IsPresent\(IT\) && \(\@IT!=Stamp\)\)\)\]//g' ./tsv/1.4/AnnotStamp-BEFORE.tsv > ./tsv/1.4/AnnotStamp.tsv
	rm ./tsv/1.4/AnnotStamp-BEFORE.tsv

	mv ./tsv/1.5/AnnotStamp.tsv ./tsv/1.5/AnnotStamp-BEFORE.tsv
	sed -E 's/\[fn\:Not\(fn\:IsRequired\(fn\:IsPresent\(IT\) && \(\@IT!=Stamp\)\)\)\]//g' ./tsv/1.5/AnnotStamp-BEFORE.tsv > ./tsv/1.5/AnnotStamp.tsv
	rm ./tsv/1.5/AnnotStamp-BEFORE.tsv

	mv ./tsv/1.6/AnnotStamp.tsv ./tsv/1.6/AnnotStamp-BEFORE.tsv
	sed -E 's/\[fn\:Not\(fn\:IsRequired\(fn\:IsPresent\(IT\) && \(\@IT!=Stamp\)\)\)\]//g' ./tsv/1.6/AnnotStamp-BEFORE.tsv > ./tsv/1.6/AnnotStamp.tsv
	rm ./tsv/1.6/AnnotStamp-BEFORE.tsv

	mv ./tsv/1.7/AnnotStamp.tsv ./tsv/1.7/AnnotStamp-BEFORE.tsv
	sed -E 's/\[fn\:Not\(fn\:IsRequired\(fn\:IsPresent\(IT\) && \(\@IT!=Stamp\)\)\)\]//g' ./tsv/1.7/AnnotStamp-BEFORE.tsv > ./tsv/1.7/AnnotStamp.tsv
	rm ./tsv/1.7/AnnotStamp-BEFORE.tsv

	mv ./tsv/1.3/DeviceNDict.tsv ./tsv/1.3/DeviceNDict-BEFORE.tsv
	sed -E 's/fn:IsRequired\(fn:SinceVersion\(1.6,\(@Subtype==NChannel\)\) && fn:HasSpotColorants\(parent::1\)\)/FALSE/g' ./tsv/1.3/DeviceNDict-BEFORE.tsv > ./tsv/1.3/DeviceNDict.tsv
	rm ./tsv/1.3/DeviceNDict-BEFORE.tsv

	mv ./tsv/1.4/DeviceNDict.tsv ./tsv/1.4/DeviceNDict-BEFORE.tsv
	sed -E 's/fn:IsRequired\(fn:SinceVersion\(1.6,\(@Subtype==NChannel\)\) && fn:HasSpotColorants\(parent::1\)\)/FALSE/g' ./tsv/1.4/DeviceNDict-BEFORE.tsv > ./tsv/1.4/DeviceNDict.tsv
	rm ./tsv/1.4/DeviceNDict-BEFORE.tsv

	mv ./tsv/1.5/DeviceNDict.tsv ./tsv/1.5/DeviceNDict-BEFORE.tsv
	sed -E 's/fn:IsRequired\(fn:SinceVersion\(1.6,\(@Subtype==NChannel\)\) && fn:HasSpotColorants\(parent::1\)\)/FALSE/g' ./tsv/1.5/DeviceNDict-BEFORE.tsv > ./tsv/1.5/DeviceNDict.tsv
	rm ./tsv/1.5/DeviceNDict-BEFORE.tsv

	mv ./tsv/1.1/Transition.tsv ./tsv/1.1/Transition-BEFORE.tsv
	sed -E 's/ && fn:SinceVersion\(1.5,\(@SS!=1.0\)\)//g' ./tsv/1.1/Transition-BEFORE.tsv > ./tsv/1.1/Transition.tsv
	rm ./tsv/1.1/Transition-BEFORE.tsv

	mv ./tsv/1.2/Transition.tsv ./tsv/1.2/Transition-BEFORE.tsv
	sed -E 's/ && fn:SinceVersion\(1.5,\(@SS!=1.0\)\)//g' ./tsv/1.2/Transition-BEFORE.tsv > ./tsv/1.2/Transition.tsv
	rm ./tsv/1.2/Transition-BEFORE.tsv

	mv ./tsv/1.3/Transition.tsv ./tsv/1.3/Transition-BEFORE.tsv
	sed -E 's/ && fn:SinceVersion\(1.5,\(@SS!=1.0\)\)//g' ./tsv/1.3/Transition-BEFORE.tsv > ./tsv/1.3/Transition.tsv
	rm ./tsv/1.3/Transition-BEFORE.tsv

	mv ./tsv/1.4/Transition.tsv ./tsv/1.4/Transition-BEFORE.tsv
	sed -E 's/ && fn:SinceVersion\(1.5,\(@SS!=1.0\)\)//g' ./tsv/1.4/Transition-BEFORE.tsv > ./tsv/1.4/Transition.tsv
	rm ./tsv/1.4/Transition-BEFORE.tsv

	mv ./tsv/1.3/AnnotStamp.tsv ./tsv/1.3/AnnotStamp-BEFORE.tsv
	sed -E 's/ && \(@IT!=Stamp\)//g' ./tsv/1.3/AnnotStamp-BEFORE.tsv > ./tsv/1.3/AnnotStamp.tsv
	rm ./tsv/1.3/AnnotStamp-BEFORE.tsv

	mv ./tsv/1.4/AnnotStamp.tsv ./tsv/1.4/AnnotStamp-BEFORE.tsv
	sed -E 's/ && \(@IT!=Stamp\)//g' ./tsv/1.4/AnnotStamp-BEFORE.tsv > ./tsv/1.4/AnnotStamp.tsv
	rm ./tsv/1.4/AnnotStamp-BEFORE.tsv

	mv ./tsv/1.5/AnnotStamp.tsv ./tsv/1.5/AnnotStamp-BEFORE.tsv
	sed -E 's/ && \(@IT!=Stamp\)//g' ./tsv/1.5/AnnotStamp-BEFORE.tsv > ./tsv/1.5/AnnotStamp.tsv
	rm ./tsv/1.5/AnnotStamp-BEFORE.tsv

	mv ./tsv/1.6/AnnotStamp.tsv ./tsv/1.6/AnnotStamp-BEFORE.tsv
	sed -E 's/ && \(@IT!=Stamp\)//g' ./tsv/1.6/AnnotStamp-BEFORE.tsv > ./tsv/1.6/AnnotStamp.tsv
	rm ./tsv/1.6/AnnotStamp-BEFORE.tsv

	mv ./tsv/1.7/AnnotStamp.tsv ./tsv/1.7/AnnotStamp-BEFORE.tsv
	sed -E 's/ && \(@IT!=Stamp\)//g' ./tsv/1.7/AnnotStamp-BEFORE.tsv > ./tsv/1.7/AnnotStamp.tsv
	rm ./tsv/1.7/AnnotStamp-BEFORE.tsv

	mv ./tsv/1.4/XObjectImage.tsv ./tsv/1.4/XObjectImage-BEFORE.tsv
	sed -E 's/\[fn:SinceVersion\(1.5,fn:Not\(fn:IsPresent\(@SMaskInData>0\)\)\)\]//g' ./tsv/1.4/XObjectImage-BEFORE.tsv > ./tsv/1.4/XObjectImage.tsv
	rm ./tsv/1.4/XObjectImage-BEFORE.tsv

	TestGrammar --tsvdir ./tsv/1.0/ --validate
	python3 ./scripts/arlington.py --tsvdir ./tsv/1.0/ --validate
	TestGrammar --tsvdir ./tsv/1.1/ --validate
	python3 ./scripts/arlington.py --tsvdir ./tsv/1.1/ --validate
	TestGrammar --tsvdir ./tsv/1.2/ --validate
	python3 ./scripts/arlington.py --tsvdir ./tsv/1.2/ --validate
	TestGrammar --tsvdir ./tsv/1.3/ --validate
	python3 ./scripts/arlington.py --tsvdir ./tsv/1.3/ --validate
	TestGrammar --tsvdir ./tsv/1.4/ --validate
	python3 ./scripts/arlington.py --tsvdir ./tsv/1.4/ --validate
	TestGrammar --tsvdir ./tsv/1.5/ --validate
	python3 ./scripts/arlington.py --tsvdir ./tsv/1.5/ --validate
	TestGrammar --tsvdir ./tsv/1.6/ --validate
	python3 ./scripts/arlington.py --tsvdir ./tsv/1.6/ --validate
	TestGrammar --tsvdir ./tsv/1.7/ --validate
	python3 ./scripts/arlington.py --tsvdir ./tsv/1.7/ --validate
	TestGrammar --tsvdir ./tsv/2.0/ --validate
	python3 ./scripts/arlington.py --tsvdir ./tsv/2.0/ --validate
	TestGrammar --tsvdir ./tsv/latest/ --validate
	python3 ./scripts/arlington.py --tsvdir ./tsv/latest/ --validate


# Create all TSV file sets for each PDF version based on tsv/latest using Java PoC app. SLOW!
.PHONY: tsv
tsv: ./gcxml/dist/Gcxml.jar
	java -jar ./gcxml/dist/Gcxml.jar -tsv


# Make the Java PoC app, run it and then validate the generated XML
xml: ./xml/pdf_grammar1.0.xml ./xml/pdf_grammar1.1.xml ./xml/pdf_grammar1.2.xml ./xml/pdf_grammar1.3.xml \
	./xml/pdf_grammar1.4.xml ./xml/pdf_grammar1.5.xml ./xml/pdf_grammar1.6.xml ./xml/pdf_grammar1.7.xml ./xml/pdf_grammar2.0.xml
	${XMLLINT} ${XMLLINT_FLAGS} --schema ./xml/schema/arlington-pdf.xsd $?


# Create and validate XML files for each PDF version based on tsv/latest using the Java PoC app. SLOW!
xml/%.xml: ./gcxml/dist/Gcxml.jar
	echo "Creating XML: $(strip $(subst xml/pdf_grammar,,$(subst .xml,,$@)))"
	java -jar ./gcxml/dist/Gcxml.jar -xml $(strip $(subst xml/pdf_grammar,,$(subst .xml,,$@)))


# Build the Java proof-of-concept application using "ant"
./gcxml/dist/Gcxml.jar:
	( cd gcxml ; ant ; cd .. )
