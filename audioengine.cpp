/*
	Copyright (C) 2019-2021 Doug McLain

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "audioengine.h"
#include <QDebug>
#include <cmath>

#ifdef Q_OS_MACOS
#define MACHAK 1
#else
#define MACHAK 0
#endif

//AudioEngine::AudioEngine(QObject *parent) : QObject(parent)
AudioEngine::AudioEngine(QString in, QString out) :
	m_outputdevice(out),
	m_inputdevice(in),
	m_out(nullptr),
	m_in(nullptr),
	m_srm(1)
{
	m_audio_out_temp_buf_p = m_audio_out_temp_buf;
	memset(m_aout_max_buf, 0, sizeof(float) * 200);
	m_aout_max_buf_p = m_aout_max_buf;
	m_aout_max_buf_idx = 0;
	m_aout_gain = 100;
	m_volume = 1.0f;
}

AudioEngine::~AudioEngine()
{
	//m_indev->disconnect();
	//m_in->stop();
	//m_outdev->disconnect();
	//m_out->stop();
	//delete m_in;
	//delete m_out;
}

QStringList AudioEngine::discover_audio_devices(uint8_t d)
{
	QStringList list;
	QAudio::Mode m = (d) ? QAudio::AudioOutput :  QAudio::AudioInput;
	QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(m);

	for (QList<QAudioDeviceInfo>::ConstIterator it = devices.constBegin(); it != devices.constEnd(); ++it ) {
		//fprintf(stderr, "Playback device name = %s\n", (*it).deviceName().toStdString().c_str());fflush(stderr);
		list.append((*it).deviceName());
	}
	return list;
}

void AudioEngine::init()
{
	QAudioFormat format;
	QAudioFormat tempformat;
	format.setSampleRate(8000);
	format.setChannelCount(1);
	format.setSampleSize(16);
	format.setCodec("audio/pcm");
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setSampleType(QAudioFormat::SignedInt);

	m_agc = true;

	QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);

	if(devices.size() == 0){
		fprintf(stderr, "No audio playback hardware found\n");fflush(stderr);
	}
	else{
		QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
		for (QList<QAudioDeviceInfo>::ConstIterator it = devices.constBegin(); it != devices.constEnd(); ++it ) {
			if(MACHAK){
				qDebug() << "Playback device name = " << (*it).deviceName();
				qDebug() << (*it).supportedByteOrders();
				qDebug() << (*it).supportedChannelCounts();
				qDebug() << (*it).supportedCodecs();
				qDebug() << (*it).supportedSampleRates();
				qDebug() << (*it).supportedSampleSizes();
				qDebug() << (*it).supportedSampleTypes();
				qDebug() << (*it).preferredFormat();
			}
			if((*it).deviceName() == m_outputdevice){
				info = *it;
			}
		}
		if (!info.isFormatSupported(format)) {
			qWarning() << "Raw audio format not supported by backend, trying nearest format.";
			tempformat = info.nearestFormat(format);
			qWarning() << "Format now set to " << format.sampleRate() << ":" << format.sampleSize();
		}
		else{
			tempformat = format;
		}
		fprintf(stderr, "Using playback device %s\n", info.deviceName().toStdString().c_str());fflush(stderr);

		m_out = new QAudioOutput(info, tempformat, this);
		m_out->setBufferSize(19200);
		connect(m_out, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));
		//m_outdev = m_out->start();
	}

	devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);

	if(devices.size() == 0){
		fprintf(stderr, "No audio recording hardware found\n");fflush(stderr);
	}
	else{
		QAudioDeviceInfo info(QAudioDeviceInfo::defaultInputDevice());
		for (QList<QAudioDeviceInfo>::ConstIterator it = devices.constBegin(); it != devices.constEnd(); ++it ) {
			if(MACHAK){
				qDebug() << "Capture device name = " << (*it).deviceName();
				qDebug() << (*it).supportedByteOrders();
				qDebug() << (*it).supportedChannelCounts();
				qDebug() << (*it).supportedCodecs();
				qDebug() << (*it).supportedSampleRates();
				qDebug() << (*it).supportedSampleSizes();
				qDebug() << (*it).supportedSampleTypes();
				qDebug() << (*it).preferredFormat();
			}
			if((*it).deviceName() == m_inputdevice){
				info = *it;
			}
		}
		if (!info.isFormatSupported(format)) {
			qWarning() << "Raw audio format not supported by backend, trying nearest format.";
			tempformat = info.nearestFormat(format);
			qWarning() << "Format now set to " << format.sampleRate() << ":" << format.sampleSize();
		}
		else{
			tempformat = format;
		}

		int sr = 8000;
		if(MACHAK){
			sr = info.preferredFormat().sampleRate();
			m_srm = (float)sr / 8000.0;
		}
		format.setSampleRate(sr);
		m_in = new QAudioInput(info, format, this);
		fprintf(stderr, "Capture device: %s SR: %d resample factor: %f\n", info.deviceName().toStdString().c_str(), sr, m_srm);fflush(stderr);
	}
}

void AudioEngine::start_capture()
{
	m_audioinq.clear();
	if(m_in != nullptr){
		m_indev = m_in->start();
		connect(m_indev, SIGNAL(readyRead()), SLOT(input_data_received()));
	}
}

void AudioEngine::stop_capture()
{
	if(m_in != nullptr){
		m_indev->disconnect();
		m_in->stop();
	}
}

void AudioEngine::start_playback()
{
	//m_out->reset();
	m_outdev = m_out->start();
}

void AudioEngine::stop_playback()
{
	m_out->stop();
}

void AudioEngine::input_data_received()
{
	QByteArray data;
	qint64 len = m_in->bytesReady();

	if (len > 0){
		data.resize(len);
		m_indev->read(data.data(), len);
/*
		fprintf(stderr, "AUDIOIN: ");
		for(int i = 0; i < len; ++i){
			fprintf(stderr, "%02x ", (unsigned char)data.data()[i]);
		}
		fprintf(stderr, "\n");
		fflush(stderr);
*/
		if(MACHAK){
			std::vector<int16_t> samples;
			for(int i = 0; i < len; i += 2){
				samples.push_back(((data.data()[i+1] << 8) & 0xff00) | (data.data()[i] & 0xff));
			}
			for(float i = 0; i < (float)len/2; i += m_srm){
				m_audioinq.enqueue(samples[i]);
			}
		}
		else{
			for(int i = 0; i < len; i += (2 * m_srm)){
				m_audioinq.enqueue(((data.data()[i+1] << 8) & 0xff00) | (data.data()[i] & 0xff));
			}
		}
	}
}

void AudioEngine::write(int16_t *pcm, size_t s)
{
	m_maxlevel = 0;
/*
	fprintf(stderr, "AUDIOOUT: ");
	for(int i = 0; i < s; ++i){
		fprintf(stderr, "%04x ", (uint16_t)pcm[i]);
	}
	fprintf(stderr, "\n");
	fflush(stderr);
*/
	if(m_agc){
		process_audio(pcm, s);
	}

	m_outdev->write((const char *) pcm, sizeof(int16_t) * s);
	for(uint32_t i = 0; i < s; ++i){
		if(pcm[i] > m_maxlevel){
			m_maxlevel = pcm[i];
		}
	}
}

uint16_t AudioEngine::read(int16_t *pcm, int s)
{
	m_maxlevel = 0;

	if(m_audioinq.size() >= s){
		for(int i = 0; i < s; ++i){
			pcm[i] = m_audioinq.dequeue();
			if(pcm[i] > m_maxlevel){
				m_maxlevel = pcm[i];
			}
		}
		return 1;
	}
	else if(m_in == nullptr){
		memset(pcm, 0, sizeof(int16_t) * s);
		return 1;
	}
	else{
		//fprintf(stderr, "audio frame not avail size == %d\n", m_audioinq.size());
		return 0;
	}
}

uint16_t AudioEngine::read(int16_t *pcm)
{
	int s;
	m_maxlevel = 0;

	if(m_audioinq.size() >= 160){
		s = 160;
	}
	else{
		s = m_audioinq.size();
	}

	for(int i = 0; i < s; ++i){
		pcm[i] = m_audioinq.dequeue();
		if(pcm[i] > m_maxlevel){
			m_maxlevel = pcm[i];
		}
	}

	return s;
}

// process_audio() based on code from DSD https://github.com/szechyjs/dsd
void AudioEngine::process_audio(int16_t *pcm, size_t s)
{
	float aout_abs, max, gainfactor, gaindelta, maxbuf;

	for(size_t i = 0; i < s; ++i){
		m_audio_out_temp_buf[i] = static_cast<float>(pcm[i]);
	}

	// detect max level
	max = 0;
	m_audio_out_temp_buf_p = m_audio_out_temp_buf;

	for (size_t i = 0; i < s; i++){
		aout_abs = fabsf(*m_audio_out_temp_buf_p);

		if (aout_abs > max){
			max = aout_abs;
		}

		m_audio_out_temp_buf_p++;
	}

	*m_aout_max_buf_p = max;
	m_aout_max_buf_p++;
	m_aout_max_buf_idx++;

	if (m_aout_max_buf_idx > 24){
		m_aout_max_buf_idx = 0;
		m_aout_max_buf_p = m_aout_max_buf;
	}

	// lookup max history
	for (size_t i = 0; i < 25; i++){
		maxbuf = m_aout_max_buf[i];

		if (maxbuf > max){
			max = maxbuf;
		}
	}

	// determine optimal gain level
	if (max > static_cast<float>(0)){
		gainfactor = (static_cast<float>(30000) / max);
	}
	else{
		gainfactor = static_cast<float>(50);
	}

	if (gainfactor < m_aout_gain){
		m_aout_gain = gainfactor;
		gaindelta = static_cast<float>(0);
	}
	else{
		if (gainfactor > static_cast<float>(50)){
			gainfactor = static_cast<float>(50);
		}

		gaindelta = gainfactor - m_aout_gain;

		if (gaindelta > (static_cast<float>(0.05) * m_aout_gain)){
			gaindelta = (static_cast<float>(0.05) * m_aout_gain);
		}
	}

	gaindelta /= static_cast<float>(160);

	// adjust output gain
	m_audio_out_temp_buf_p = m_audio_out_temp_buf;

	for (size_t i = 0; i < 160; i++){
		*m_audio_out_temp_buf_p = (m_aout_gain + (static_cast<float>(i) * gaindelta)) * (*m_audio_out_temp_buf_p);
		m_audio_out_temp_buf_p++;
	}

	m_aout_gain += (static_cast<float>(s) * gaindelta);
	m_audio_out_temp_buf_p = m_audio_out_temp_buf;

	for (size_t i = 0; i < s; i++){
		*m_audio_out_temp_buf_p *= m_volume;
		if (*m_audio_out_temp_buf_p > static_cast<float>(32760)){
			*m_audio_out_temp_buf_p = static_cast<float>(32760);
		}
		else if (*m_audio_out_temp_buf_p < static_cast<float>(-32760)){
			*m_audio_out_temp_buf_p = static_cast<float>(-32760);
		}
		pcm[i] = static_cast<int16_t>(*m_audio_out_temp_buf_p);
		m_audio_out_temp_buf_p++;
	}
}

void AudioEngine::handleStateChanged(QAudio::State newState)
{
	switch (newState) {
	case QAudio::ActiveState:
		//qDebug() << "AudioOut state active";
		break;
	case QAudio::SuspendedState:
		//qDebug() << "AudioOut state suspended";
		break;
	case QAudio::IdleState:
		//qDebug() << "AudioOut state idle";
		break;
	case QAudio::StoppedState:
		//qDebug() << "AudioOut state stopped";
		break;
	default:
		break;
	}
}
