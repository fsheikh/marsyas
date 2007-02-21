/*
** Copyright (C) 1998-2006 George Tzanetakis <gtzan@cs.uvic.ca>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/** 
\class PeClusters
\brief 

*/

#include "PeClusters.h"
#include "PeUtilities.h"
#include "FileName.h"

using namespace std;
using namespace Marsyas;



PeCluster::PeCluster()
{
	groundLabel = -1;
	oriLabel=-1;
	label=-1;
	histSize = 100;
}

PeCluster::~PeCluster()
{

}

void
PeCluster::init(realvec& peakSet, mrs_natural l)
{
	mrs_natural i;

	// set general informations
	start = MAXNATURAL;
	end=0;
	nbPeaks=0;
	for (i=0 ; i< peakSet.getRows() ; i++)
		if (peakSet(i, pkGroup) == l)
		{
			if(peakSet(i, pkTime) < start)
			{
				start = peakSet(i, pkTime);
			}
			if(peakSet(i, pkTime) > end)
			{
				end= peakSet(i, pkTime);
			}
			nbPeaks++;
		}

		length = end-start;
		envSize = (mrs_natural) length+1;
		oriLabel = l;
		label = l;
}

void 
PeCluster::computeAttributes(realvec& peakSet, mrs_natural l, string type, mrs_real cuttingFrequency)
{
	mrs_natural i, j, k;
	realvec set;

	set.stretch(nbPeaks, nbPkParameters);

	for (i=0, k=0 ; i< peakSet.getRows() ; i++)
		if (peakSet(i, pkGroup) == l)
		{
			for(j=0 ; j<nbPkParameters ; j++)
				set(k, j) = peakSet(i, j);
			k++;
		}

		// compute stats
		freqMean = set.getCol(pkFrequency).mean();
		freqStd = set.getCol(pkFrequency).std();
		ampMean = set.getCol(pkAmplitude).mean();
		ampStd = set.getCol(pkAmplitude).std();

		// compute envelopes
		frequencyEvolution.stretch(envSize);
		frequencyEvolution.setval(0);
		amplitudeEvolution.stretch(envSize);
		amplitudeEvolution.setval(0);

		for(i=0 ; i<nbPeaks ; i++)
		{ 
			frequencyEvolution(((mrs_natural) set(i, pkTime)-start)) += 
				set(i, pkFrequency);//*set(i, pkAmplitude);

			amplitudeEvolution(((mrs_natural) set(i, pkTime)-start)) += 
				set(i, pkAmplitude);
		}

		amplitudeHistogram.stretch(histSize);
		amplitudeHistogram.setval(0);
		frequencyHistogram.stretch(histSize);
		frequencyHistogram.setval(0);
		harmonicityHistogram.stretch(histSize);
		harmonicityHistogram.setval(0);
		// compute histograms
		mrs_real norm = 0;
		for(i=0 ; i<nbPeaks ; i++)
		{
			amplitudeHistogram((mrs_natural) floor(set(i, pkAmplitude)*histSize)) += 1;
			frequencyHistogram((mrs_natural) floor(set(i, pkFrequency)*histSize/cuttingFrequency)) += set(i, pkAmplitude);
			norm+= set(i,pkAmplitude);
		}
		//normalize them
		amplitudeHistogram/=norm;
		// frequencyHistogram/=norm;
		// compute similarities within cluster

		cuttingFrequency_ = cuttingFrequency;

		extractParameter(set, frequencySet_, pkFrequency, 60);
	  extractParameter(set, amplitudeSet_, pkAmplitude, 60);
}

mrs_natural 
PeCluster::getVecSize()
{
	return 3+6+1+2*(envSize+histSize);
}

void 
PeCluster::toVec(realvec& vec)
{
	mrs_natural i=0, j;

	vec(i++) = oriLabel;
	vec(i++) = groundLabel;	
	vec(i++) = label;

	vec(i++) = start;
	vec(i++) = length;	
	vec(i++) = freqMean;
	vec(i++) = freqStd;
	vec(i++) = ampMean;
	vec(i++) = ampStd;
	vec(i++) = getVoicingFactor1(0);

	for (j=0;j<envSize ; j++)
		vec(i++) = frequencyEvolution(j);
	for (j=0;j<envSize ; j++)
		vec(i++) = amplitudeEvolution(j);
	for (j=0;j<histSize ; j++)
		vec(i++) = frequencyHistogram(j);
	for (j=0;j<histSize ; j++)
		vec(i++) = amplitudeHistogram(j);
}

mrs_natural 
PeCluster::getGroundThruth ()
{
	return groundLabel;
}

void 
PeCluster::setGroundThruth (mrs_natural l)
{
	groundLabel = l;
}

mrs_natural 
PeCluster::getOriLabel ()
{
	return oriLabel;
}

void PeCluster::setOriLabel (mrs_natural l)
{
	oriLabel = l;
}

mrs_natural PeCluster::getLabel ()
{
	return label;
}

void PeCluster::setLabel (mrs_natural l)
{
	label = l;
}


mrs_real 
PeCluster::getVoicingFactor3(mrs_natural fg)
{
	//	return frequencyHistogram.maxval();

	mrs_natural i, index=0, indexStart=0, indexEnd=histSize, interval= (mrs_natural) ceil(histSize/400.0);
	mrs_real maxVal=0, sum=0;
	// look for the maximum
	for (i=0 ; i<histSize ; i++)
		if (maxVal < frequencyHistogram(i))
		{
			index = i;
			maxVal = frequencyHistogram(i);
		}

		if (index-interval<0)
			indexStart = 0;
		else
			indexStart = index-interval;

		if (index+interval>histSize)
			indexStart = histSize;
		else
			indexStart = index+interval;

		// seek the neighbors
		for (i = indexStart ; i< indexEnd ; i++)
		{
			sum += frequencyHistogram(i);
		}
		//
		return sum/(2*interval);
}
//
//mrs_real PeCluster::getF0(mrs_natural fIndex){
//
//	// get max from frequencyHistogram
//	mrs_natural i, index=0, indexStart=0, indexEnd=histSize, interval= (mrs_natural) ceil(histSize/400.0);
//	mrs_real maxVal=0, sum=0;
//	// look for the maximum
//	for (i=0 ; i<histSize ; i++)
//		if (maxVal < frequencyHistogram(i))
//		{
//			index = i;
//			maxVal = frequencyHistogram(i);
//		}
//		return index/(histSize+.0)*cuttingFrequency_;
//
//		// get the frequency of the closest peaks or use histValue if too far
//
//		// look for better candidate using HWPS
//
//}
//

mrs_real PeCluster::getVoicingFactor2(mrs_natural fIndex)
{
	mrs_natural i;
realvec vFactor(amplitudeSet_.size());

for (i=0 ; i<amplitudeSet_.size() ; i++)
{

	// fix this !!!!!
	realvec vec = amplitudeSet_[i];
  // seek for the highest amplitude peak in the frame
  mrs_real maxA = vec.maxval();
  // normalize all data
  vec/=maxA;
	// compute the ratio
	vFactor(i) = maxA/vec.median();
}
return vFactor.mean();
}






mrs_real PeCluster::getVoicingFactor1(mrs_natural fIndex)
{
	mrs_natural i;
realvec vFactor(amplitudeSet_.size());

for (i=0 ; i<amplitudeSet_.size() ; i++)
{

	// fix this !!!!!
	realvec vec = amplitudeSet_[i];
  // seek for the highest amplitude peak in the frame
  mrs_real maxA = vec.maxval();
  // normalize all data
  vec/=maxA;
	// compute the ratio
	vFactor(i) = maxA/vec.mean();
}
return vFactor.mean();
}

mrs_real PeCluster::getF0(mrs_natural fIndex)
{
	mrs_natural i, indexMax;
realvec f0(amplitudeSet_.size());

for (i=0 ; i<amplitudeSet_.size() ; i++)
{
	realvec vec = frequencySet_[i];
  // seek for the highest amplitude peak in the frame
  mrs_real maxA = amplitudeSet_[i].maxval(&indexMax);
 
	f0(i) = vec(indexMax);
	// compute HWPS for every couple with this peak
}


return f0.mean();
}
///////////////////////////////////////////////////////////

//////// Clusters STUFF

////////////////////////////////////////////////////////////

PeClusters::PeClusters(realvec &peakSet)
{
	// determine the number of clusters
	nbClusters=0;
	nbFrames=0;
	for (int i=0 ; i<peakSet.getRows() ; i++)
	{
		//  cout << peakSet(i, pkGroup) << " ";
		if(peakSet(i, pkGroup) > nbClusters)
			nbClusters = (mrs_natural) peakSet(i, pkGroup);
		if(peakSet(i, pkTime) > nbFrames)
			nbFrames = (mrs_natural) peakSet(i, pkTime);
	}
	nbClusters++;
	nbFrames++;
	// build the vector of clusters
	set = new PeCluster[nbClusters];
	for (int i=0 ; i<nbClusters ; i++)
	{
		set[i].init(peakSet, i);
	}
}

PeClusters::~PeClusters(){
	delete [] set;
}
void 
PeClusters::attributes(realvec &peakSet, mrs_real cuttingFrequency)
{
	for (int i=0 ; i<nbClusters ; i++)
		set[i].computeAttributes(peakSet, i, "", cuttingFrequency);
}

void
PeClusters::getVecs(realvec& vecs)
{
	mrs_natural maxSize=0;
	for (int i=0 ; i<nbClusters ; i++)
		if(set[i].getVecSize()>maxSize)
		{
			maxSize = set[i].getVecSize();
		}
		vecs.stretch(nbClusters, maxSize);
		vecs.setval(0);

		for (int i=0 ; i<nbClusters ; i++)
		{
			realvec vec(set[i].getVecSize());
			set[i].toVec(vec);
			for (int j=0 ; j<vec.getSize() ; j++)
				vecs(i, j) = vec(j);
		}
}


void
PeClusters::getConversionTable(realvec& conversion)
{
	conversion.stretch(nbClusters+2);
	conversion(0) = -2;
	conversion(1) = -1;
	for (int i= 0; i<nbClusters ; i++)
	{
		conversion(i+2) = set[i].label;
	}
}

void
PeClusters::selectBefore(mrs_real val)
{
	for (int i=0 ; i<nbClusters ; i++)
	{
		if(set[i].start < val)
			set[i].label = 0;
		else
			set[i].label = -1;
	}
}

void
PeClusters::selectGround()
{
	for (int i=0 ; i<nbClusters ; i++)
	{
		set[i].label = set[i].groundLabel;
	}
}


mrs_real
PeClusters::synthetize(realvec &peakSet, string fileName, string outFileName, mrs_natural Nw, mrs_natural D, mrs_natural S, mrs_natural bopt, mrs_natural synType, mrs_natural residual)
{
	mrs_real snrVal=0;
	cout << "Synthetizing Clusters" << endl;
	MarSystemManager mng;

	synthNetCreate(&mng, fileName, 0, 1);
	MarSystem* pvseries = mng.create("Series", "pvseries");
	MarSystem *peSynth = mng.create("PeSynthetize", "synthNet");

	MarSystem *peSource = mng.create("RealvecSource", "peSource");
	pvseries->addMarSystem(peSource);


	pvseries->addMarSystem(peSynth);

	// convert peakSet in frame form
	realvec pkV(S*nbPkParameters, nbFrames);

	// for all clusters
	cout << "number of Clusters: " << nbClusters << endl;
	for (int i=0 ; i<nbClusters ; i++)
		if(set[i].label != -1)
		{
			// label peak set
			pkV.setval(0);
		
			peaks2V(peakSet, peakSet, pkV, S, i);

			pvseries->setctrl("RealvecSource/peSource/mrs_realvec/data", pkV);
			pvseries->setctrl("RealvecSource/peSource/mrs_real/israte", peakSet(0, 1));

			// configure synthesis
			FileName FileName(outFileName);
			string path = FileName.path();
			string name = FileName.nameNoExt();
			string ext = FileName.ext();

			ostringstream ossi;
			ossi << i;
			string outsfname;
			string fileResName;
			if(!residual)
			{
				outsfname = path + name +  "_" +  ossi.str() + "." + ext;
				fileResName = path + name +  "Res_" + ossi.str() + "." + ext;
			}
			else
			{
				/*		outsfname = path + name + "Grd_" +  ossi.str() + "." + ext;
				fileResName = path + name + "GrdRes_" + ossi.str() + "." + ext;*/
			}

			synthNetConfigure (pvseries, fileName, outsfname, fileResName, 1, Nw, D, S, 1, 0, 0, bopt, Nw+1-D); //  nbFrames

			mrs_natural nbActiveFrames=0;
			mrs_real Snr=0;
			while(1)
			{
				// launch synthesis
				pvseries->tick();

				// get snr and decide ground thruth
				if(residual)
				{
					const mrs_real snr = pvseries->getctrl("PeSynthetize/synthNet/Series/postNet/PeResidual/res/mrs_real/snr")->toReal();
					//	cout << " SNR : "<< snr << endl;
					if(snr != -80)
					{
						nbActiveFrames++;
						Snr+=snr;
					}
				}
				if(pvseries->getctrl("RealvecSource/peSource/mrs_bool/done")->toBool())
					break;
			}

			if(Snr)
			{
				Snr/=nbActiveFrames;
				Snr = 10*log10(pow(10, Snr)-1);
			}
			else
				Snr = -80;
			cout << " SNR for cluster " << i << " : "<< Snr << " " << nbActiveFrames << endl;
			if(!i)
				snrVal = Snr;
			if(residual)
				if(Snr > -3)
					set[i].groundLabel = 0;
				else
					set[i].groundLabel = 1;
		}
		return snrVal;
}



void 
PeClusters::voicingLine(string fileName, mrs_natural hopSize, mrs_natural twLength){

	ofstream lineFile;
	lineFile.open(fileName.c_str());

	for (mrs_natural i=0 ; i<nbClusters ; i++)
	{
			for(mrs_natural j=0 ; j<twLength ; j++)
		{
		lineFile << set[i].start*hopSize << " " << set[i].getVoicingFactor1(j)<< " " << set[i].getVoicingFactor2(j)<< " " << set[i].getVoicingFactor3(j) << endl;
			}
	}
	lineFile.close();
}

void 
PeClusters::f0Line(string fileName, mrs_natural hopSize, mrs_real samplingFrequency, mrs_natural twLength){

	ofstream lineFile;
	lineFile.open(fileName.c_str());

	for (mrs_natural i=0 ; i<nbClusters ; i++)
		for(mrs_natural j=0 ; j<twLength ; j++)
		{
			cout << (set[i].start+j)*hopSize/samplingFrequency << " " << set[i].getF0(j)*samplingFrequency << endl;
			lineFile << (set[i].start+j)*hopSize/samplingFrequency << " " << set[i].getF0(j)*samplingFrequency << endl;
		}
		lineFile.close();
}
