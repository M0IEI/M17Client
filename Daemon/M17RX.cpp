/*
 *	Copyright (C) 2020,2021 Jonathan Naylor, G4KLX
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 */

#include "M17RX.h"
#include "M17Convolution.h"
#include "Golay24128.h"
#include "M17Utils.h"
#include "M17CRC.h"
#include "Utils.h"
#include "Log.h"

#include <cstdio>
#include <cassert>
#include <cstring>
#include <ctime>

const unsigned int  SILENCE_BLOCK_COUNT = 5U;

const unsigned int  BLEEP_FREQ   = 2000U;
const unsigned int  BLEEP_LENGTH = 70U;
const float         BLEEP_AMPL   = 0.01F;

const unsigned int INTERLEAVER[] = {
	0U, 137U, 90U, 227U, 180U, 317U, 270U, 39U, 360U, 129U, 82U, 219U, 172U, 309U, 262U, 31U, 352U, 121U, 74U, 211U, 164U,
	301U, 254U, 23U, 344U, 113U, 66U, 203U, 156U, 293U, 246U, 15U, 336U, 105U, 58U, 195U, 148U, 285U, 238U, 7U, 328U, 97U,
	50U, 187U, 140U, 277U, 230U, 367U, 320U, 89U, 42U, 179U, 132U, 269U, 222U, 359U, 312U, 81U, 34U, 171U, 124U, 261U, 214U,
	351U, 304U, 73U, 26U, 163U, 116U, 253U, 206U, 343U, 296U, 65U, 18U, 155U, 108U, 245U, 198U, 335U, 288U, 57U, 10U, 147U,
	100U, 237U, 190U, 327U, 280U, 49U, 2U, 139U, 92U, 229U, 182U, 319U, 272U, 41U, 362U, 131U, 84U, 221U, 174U, 311U, 264U,
	33U, 354U, 123U, 76U, 213U, 166U, 303U, 256U, 25U, 346U, 115U, 68U, 205U, 158U, 295U, 248U, 17U, 338U, 107U, 60U, 197U,
	150U, 287U, 240U, 9U, 330U, 99U, 52U, 189U, 142U, 279U, 232U, 1U, 322U, 91U, 44U, 181U, 134U, 271U, 224U, 361U, 314U, 83U,
	36U, 173U, 126U, 263U, 216U, 353U, 306U, 75U, 28U, 165U, 118U, 255U, 208U, 345U, 298U, 67U, 20U, 157U, 110U, 247U, 200U,
	337U, 290U, 59U, 12U, 149U, 102U, 239U, 192U, 329U, 282U, 51U, 4U, 141U, 94U, 231U, 184U, 321U, 274U, 43U, 364U, 133U, 86U,
	223U, 176U, 313U, 266U, 35U, 356U, 125U, 78U, 215U, 168U, 305U, 258U, 27U, 348U, 117U, 70U, 207U, 160U, 297U, 250U, 19U,
	340U, 109U, 62U, 199U, 152U, 289U, 242U, 11U, 332U, 101U, 54U, 191U, 144U, 281U, 234U, 3U, 324U, 93U, 46U, 183U, 136U, 273U,
	226U, 363U, 316U, 85U, 38U, 175U, 128U, 265U, 218U, 355U, 308U, 77U, 30U, 167U, 120U, 257U, 210U, 347U, 300U, 69U, 22U,
	159U, 112U, 249U, 202U, 339U, 292U, 61U, 14U, 151U, 104U, 241U, 194U, 331U, 284U, 53U, 6U, 143U, 96U, 233U, 186U, 323U,
	276U, 45U, 366U, 135U, 88U, 225U, 178U, 315U, 268U, 37U, 358U, 127U, 80U, 217U, 170U, 307U, 260U, 29U, 350U, 119U, 72U,
	209U, 162U, 299U, 252U, 21U, 342U, 111U, 64U, 201U, 154U, 291U, 244U, 13U, 334U, 103U, 56U, 193U, 146U, 283U, 236U, 5U,
	326U, 95U, 48U, 185U, 138U, 275U, 228U, 365U, 318U, 87U, 40U, 177U, 130U, 267U, 220U, 357U, 310U, 79U, 32U, 169U, 122U,
	259U, 212U, 349U, 302U, 71U, 24U, 161U, 114U, 251U, 204U, 341U, 294U, 63U, 16U, 153U, 106U, 243U, 196U, 333U, 286U, 55U,
	8U, 145U, 98U, 235U, 188U, 325U, 278U, 47U};

const unsigned char SCRAMBLER[] = {
	0x00U, 0x00U, 0xD6U, 0xB5U, 0xE2U, 0x30U, 0x82U, 0xFFU, 0x84U, 0x62U, 0xBAU, 0x4EU, 0x96U, 0x90U, 0xD8U, 0x98U, 0xDDU,
	0x5DU, 0x0CU, 0xC8U, 0x52U, 0x43U, 0x91U, 0x1DU, 0xF8U, 0x6EU, 0x68U, 0x2FU, 0x35U, 0xDAU, 0x14U, 0xEAU, 0xCDU, 0x76U,
	0x19U, 0x8DU, 0xD5U, 0x80U, 0xD1U, 0x33U, 0x87U, 0x13U, 0x57U, 0x18U, 0x2DU, 0x29U, 0x78U, 0xC3U};

const unsigned char BIT_MASK_TABLE[] = { 0x80U, 0x40U, 0x20U, 0x10U, 0x08U, 0x04U, 0x02U, 0x01U };

#define WRITE_BIT(p,i,b) p[(i)>>3] = (b) ? (p[(i)>>3] | BIT_MASK_TABLE[(i)&7]) : (p[(i)>>3] & ~BIT_MASK_TABLE[(i)&7])
#define READ_BIT(p,i)    (p[(i)>>3] & BIT_MASK_TABLE[(i)&7])

CM17RX::CM17RX(const std::string& callsign, CRSSIInterpolator* rssiMapper, bool bleep, CCodec2& codec3200, CCodec2& codec1600) :
m_3200(codec3200),
m_1600(codec1600),
m_callsign(callsign),
m_bleep(bleep),
m_volume(1.0F),
m_callback(NULL),
m_state(RS_RF_LISTENING),
m_frames(0U),
m_errs(0U),
m_bits(0U),
m_lsf(),
m_running(),
m_queue(25000U, "M17 RX Audio"),
m_rssiMapper(rssiMapper),
m_rssi(0U),
m_maxRSSI(0U),
m_minRSSI(0U),
m_aveRSSI(0U),
m_rssiCount(0U),
m_resampler(NULL),
m_error(0)
{
	m_resampler = ::src_new(SRC_SINC_FASTEST, 1, &m_error);
}

CM17RX::~CM17RX()
{
	::src_delete(m_resampler);
}

void CM17RX::setStatusCallback(IStatusCallback* callback)
{
	assert(callback != NULL);

	m_callback = callback;
}

unsigned int CM17RX::getVolume() const
{
	return (unsigned int)(m_volume * 100.0F + 0.5F);
}

void CM17RX::setVolume(unsigned int percentage)
{
	m_volume = float(percentage) / 100.0F;
}

unsigned int CM17RX::read(float* audio, unsigned int len)
{
	assert(audio != NULL);
	assert(len > 0U);

	if (m_queue.isEmpty())
		return 0U;

	unsigned int amt = m_queue.dataSize();
	if (len > amt)
		len = amt;

	m_queue.getData(audio, len);

	return len;
}

bool CM17RX::write(unsigned char* data, unsigned int len)
{
	assert(data != NULL);
	assert(len > 0U);

	unsigned char type = data[0U];

	if (type == TAG_LOST && (m_state == RS_RF_AUDIO || m_state == RS_RF_AUDIO_DATA)) {
		std::string source = m_lsf.getSource();
		std::string dest   = m_lsf.getDest();

		if (m_rssi != 0U)
			LogMessage("Transmission lost from %s to %s, %.1f seconds, BER: %.1f%%, RSSI: -%u/-%u/-%u dBm", source.c_str(), dest.c_str(), float(m_frames) / 25.0F, float(m_errs * 100U) / float(m_bits), m_minRSSI, m_maxRSSI, m_aveRSSI / m_rssiCount);
		else
			LogMessage("Transmission lost from %s to %s, %.1f seconds, BER: %.1f%%", source.c_str(), dest.c_str(), float(m_frames) / 25.0F, float(m_errs * 100U) / float(m_bits));
		end();
		return false;
	}

	if (type == TAG_LOST && m_state == RS_RF_DATA) {
		end();
		return false;
	}

	if (type == TAG_LOST) {
		m_state = RS_RF_LISTENING;
		return false;
	}

	// Ignore packet data
	if (type == TAG_DATA2) {
		m_state = RS_RF_LISTENING;
		return false;
	}

	// Have we got RSSI bytes on the end?
	if (len == (M17_FRAME_LENGTH_BYTES + 4U)) {
		uint16_t raw = 0U;
		raw |= (data[50U] << 8) & 0xFF00U;
		raw |= (data[51U] << 0) & 0x00FFU;

		// Convert the raw RSSI to dBm
		int rssi = m_rssiMapper->interpolate(raw);
		if (rssi != 0) {
			LogDebug("Raw RSSI: %u, reported RSSI: %d dBm", raw, rssi);
			if (m_callback != NULL)
				m_callback->rssiCallback(rssi);
		}

		// RSSI is always reported as positive
		m_rssi = (rssi >= 0) ? rssi : -rssi;

		if (m_rssi > m_minRSSI)
			m_minRSSI = m_rssi;
		if (m_rssi < m_maxRSSI)
			m_maxRSSI = m_rssi;

		m_aveRSSI += m_rssi;
		m_rssiCount++;
	}

	unsigned char temp[M17_FRAME_LENGTH_BYTES];
	decorrelator(data + 2U, temp);
	interleaver(temp, data + 2U);

	if (m_state == RS_RF_LISTENING && data[0U] == TAG_HEADER) {
		m_lsf.reset();

		CM17Convolution conv;
		unsigned char frame[M17_LSF_LENGTH_BYTES];
		unsigned int ber = conv.decodeLinkSetup(data + 2U + M17_SYNC_LENGTH_BYTES, frame);

		bool valid = CM17CRC::checkCRC16(frame, M17_LSF_LENGTH_BYTES);
		if (valid) {
			m_lsf.setLinkSetup(frame);

			bool ret = processHeader(false);
			if (!ret) {
				m_lsf.reset();
				return false;
			}

			if (m_callback != NULL) {
				m_callback->statusCallback(m_lsf.getSource(), m_lsf.getDest(), false);
				processLSF(m_lsf);
			}

			m_running.reset();
			m_frames    = 0U;
			m_errs      = ber;
			m_bits      = 368U;
			m_minRSSI   = m_rssi;
			m_maxRSSI   = m_rssi;
			m_aveRSSI   = m_rssi;
			m_rssiCount = 1U;

			addSilence(SILENCE_BLOCK_COUNT);

			LogDebug("Received link setup, BER: %u/368 (%.1f%%)", ber, float(ber) / 3.68F);

			return true;
		} else {
			m_state = RS_RF_LATE_ENTRY;
			return false;
		}
	}

	if (m_state == RS_RF_LISTENING && data[0U] == TAG_DATA1) {
		m_state = RS_RF_LATE_ENTRY;
		m_lsf.reset();
	}

	if (m_state == RS_RF_LATE_ENTRY && data[0U] == TAG_DATA1) {
		unsigned int lich1, lich2, lich3, lich4;
		bool valid1 = CGolay24128::decode24128(data + 2U + M17_SYNC_LENGTH_BYTES + 0U, lich1);
		bool valid2 = CGolay24128::decode24128(data + 2U + M17_SYNC_LENGTH_BYTES + 3U, lich2);
		bool valid3 = CGolay24128::decode24128(data + 2U + M17_SYNC_LENGTH_BYTES + 6U, lich3);
		bool valid4 = CGolay24128::decode24128(data + 2U + M17_SYNC_LENGTH_BYTES + 9U, lich4);

		if (!valid1 || !valid2 || !valid3 || !valid4)
			return false;

		unsigned char lich[M17_LICH_FRAGMENT_LENGTH_BYTES];
		CM17Utils::combineFragmentLICH(lich1, lich2, lich3, lich4, lich);

		unsigned int n = (lich4 >> 5) & 0x07U;
		m_lsf.setFragment(lich, n);

		bool valid = m_lsf.isValid();
		if (valid) {
			bool ret = processHeader(true);
			if (!ret) {
				m_lsf.reset();
				return false;
			}

			if (m_callback != NULL) {
				m_callback->statusCallback(m_lsf.getSource(), m_lsf.getDest(), false);
				processLSF(m_lsf);
			}

			m_running.reset();
			m_frames  = 0U;
			m_errs    = 0U;
			m_bits    = 1U;
			m_minRSSI = m_rssi;
			m_maxRSSI = m_rssi;
			m_aveRSSI = m_rssi;
			m_rssiCount = 1U;

			addSilence(SILENCE_BLOCK_COUNT);

			// Fall through
		} else {
			return false;
		}
	}

	if ((m_state == RS_RF_AUDIO || m_state == RS_RF_AUDIO_DATA) && data[0U] == TAG_DATA1) {
		processRunningLSF(data + 2U + M17_SYNC_LENGTH_BYTES);

		CM17Convolution conv;
		unsigned char frame[M17_FN_LENGTH_BYTES + M17_PAYLOAD_LENGTH_BYTES];
		unsigned int ber = conv.decodeData(data + 2U + M17_SYNC_LENGTH_BYTES + M17_LICH_FRAGMENT_FEC_LENGTH_BYTES, frame);

		unsigned int fn = (frame[0U] << 8) + (frame[1U] << 0);

		LogDebug("Received audio, FN: %u, BER: %u/272 (%.1f%%)", fn & 0x7FFFU, ber, float(ber) / 2.72F);

		m_bits += 272U;
		m_errs += ber;

		// A valid M17 audio frame
		short audio[CODEC_BLOCK_SIZE];
		if (m_state == RS_RF_AUDIO) {
			m_3200.codec2_decode(audio + 0U,   frame + 2U);
			m_3200.codec2_decode(audio + 160U, frame + 2U + 8U);
		} else {
			m_1600.codec2_decode(audio + 0U,   frame + 2U);
			m_1600.codec2_decode(audio + 160U, frame + 2U + 4U);
			CUtils::dump(1U, "Data Payload", frame + 2U + 8U, 8U);
		}

		// Adjust the volume, and convert to float
		float f8000[CODEC_BLOCK_SIZE];
		for (unsigned int i = 0U; i < CODEC_BLOCK_SIZE; i++)
			f8000[i] = (float(audio[i]) * m_volume) / 32768.0F;

		float f48000[SOUNDCARD_BLOCK_SIZE];

		SRC_DATA data;
		data.data_in       = f8000;
		data.data_out      = f48000;
		data.input_frames  = CODEC_BLOCK_SIZE;
		data.output_frames = SOUNDCARD_BLOCK_SIZE;
		data.end_of_input  = 0;
		data.src_ratio     = double(SOUNDCARD_SAMPLE_RATE) / double(CODEC_SAMPLE_RATE);

		int ret = ::src_process(m_resampler, &data);
		if (ret != 0)
			LogError("Error from the RX resampler - %d - %s", ret, ::src_strerror(ret));

		writeQueue(f48000, SOUNDCARD_BLOCK_SIZE);

		m_frames++;

		bool bValid = (fn & 0x7FFFU) < (210U * 25U);		// 210 seconds maximum
		bool bEnd   = (fn & 0x8000U) == 0x8000U;

		if (bValid && bEnd) {
			std::string source = m_lsf.getSource();
			std::string dest   = m_lsf.getDest();

			if (m_rssi != 0U)
				LogMessage("Received end of transmission from %s to %s, %.1f seconds, BER: %.1f%%, RSSI: -%u/-%u/-%u dBm", source.c_str(), dest.c_str(), float(m_frames) / 25.0F, float(m_errs * 100U) / float(m_bits), m_minRSSI, m_maxRSSI, m_aveRSSI / m_rssiCount);
			else
				LogMessage("Received end of transmission from %s to %s, %.1f seconds, BER: %.1f%%", source.c_str(), dest.c_str(), float(m_frames) / 25.0F, float(m_errs * 100U) / float(m_bits));
			end();
		}

		return true;
	}

	return false;
}

void CM17RX::end()
{
	if (m_state == RS_RF_AUDIO || m_state == RS_RF_AUDIO_DATA) {
		if (m_bleep)
			addBleep();

		if (m_callback != NULL)
			m_callback->statusCallback(m_lsf.getSource(), m_lsf.getDest(), true);
	}

	m_state = RS_RF_LISTENING;

	m_running.reset();
	m_lsf.reset();
}

void CM17RX::writeQueue(const float *audio, unsigned int len)
{
	assert(audio != NULL);
	assert(len > 0U);

	unsigned int space = m_queue.freeSpace();
	if (space < len) {
		LogError("Overflow in the M17 RX queue");
		return;
	}

	m_queue.addData(audio, len);
}

bool CM17RX::processHeader(bool lateEntry)
{
	unsigned char packetStream = m_lsf.getPacketStream();
	if (packetStream == M17_PACKET_TYPE)
		return false;

	std::string source = m_lsf.getSource();
	std::string dest   = m_lsf.getDest();

	unsigned char dataType = m_lsf.getDataType();
	switch (dataType) {
	case M17_DATA_TYPE_DATA:
		LogMessage("Received%sdata transmission from %s to %s", lateEntry ? " late entry " : " ", source.c_str(), dest.c_str());
		m_state = RS_RF_DATA;
		break;
	case M17_DATA_TYPE_VOICE:
		LogMessage("Received%svoice transmission from %s to %s", lateEntry ? " late entry " : " ", source.c_str(), dest.c_str());
		m_state = RS_RF_AUDIO;
		break;
	case M17_DATA_TYPE_VOICE_DATA:
		LogMessage("Received%svoice + data transmission from %s to %s", lateEntry ? " late entry " : " ", source.c_str(), dest.c_str());
		m_state = RS_RF_AUDIO_DATA;
		break;
	default:
		LogMessage("Received%sunknown transmission from %s to %s", lateEntry ? " late entry " : " ", source.c_str(), dest.c_str());
		m_state = RS_RF_DATA;
		break;
	}

	// Valid M17 LSF received

	return true;
}

void CM17RX::interleaver(const unsigned char* in, unsigned char* out) const
{
	assert(in != NULL);
	assert(out != NULL);

	for (unsigned int i = 0U; i < (M17_FRAME_LENGTH_BITS - M17_SYNC_LENGTH_BITS); i++) {
		unsigned int n1 = i + M17_SYNC_LENGTH_BITS;
		bool b = READ_BIT(in, n1) != 0U;
		unsigned int n2 = INTERLEAVER[i] + M17_SYNC_LENGTH_BITS;
		WRITE_BIT(out, n2, b);
	}
}

void CM17RX::decorrelator(const unsigned char* in, unsigned char* out) const
{
	assert(in != NULL);
	assert(out != NULL);

	for (unsigned int i = M17_SYNC_LENGTH_BYTES; i < M17_FRAME_LENGTH_BYTES; i++)
		out[i] = in[i] ^ SCRAMBLER[i];
}

void CM17RX::processLSF(const CM17LSF& lsf) const
{
	if (lsf.getEncryptionType() == M17_ENCRYPTION_TYPE_NONE) {
		char meta[20U];
		lsf.getMeta((unsigned char *)meta);

		switch (lsf.getEncryptionSubType()) {
			case M17_ENCRYPTION_SUB_TYPE_TEXT:
				CUtils::dump(1U, "LSF Text Data", (unsigned char *)meta, M17_META_LENGTH_BYTES);

				meta[M17_META_LENGTH_BYTES] = '\0';

				// Make sure that the text is valid, or at least interesting
				if (meta[0U] != 0x00U && ::strcmp(meta, "              ") != 0)
					m_callback->textCallback(meta);

				break;

			case M17_ENCRYPTION_SUB_TYPE_GPS:
				CUtils::dump(1U, "LSF GPS Data", (unsigned char *)meta, M17_META_LENGTH_BYTES);
				break;

			default:
				LogDebug("Unhandled LSF Data Type: %u", lsf.getEncryptionSubType());
				CUtils::dump(1U, "LSF Meta Data", (unsigned char *)meta, M17_META_LENGTH_BYTES);
				break;
		}
	} else {
		char meta[20U];
		lsf.getMeta((unsigned char *)meta);

		switch (lsf.getEncryptionSubType()) {
			case M17_ENCRYPTION_TYPE_AES:
				CUtils::dump(1U, "AES Encryption", (unsigned char *)meta, M17_META_LENGTH_BYTES);
				break;
			case M17_ENCRYPTION_TYPE_SCRAMBLE:
				CUtils::dump(1U, "Scrambling", (unsigned char *)meta, M17_META_LENGTH_BYTES);
				break;
			default:
				LogDebug("Unhandled Encryption Type: %u", lsf.getEncryptionType());
				CUtils::dump(1U, "LSF Meta Data", (unsigned char *)meta, M17_META_LENGTH_BYTES);
				break;
		}
	}
}

void CM17RX::processRunningLSF(const unsigned char* fragment)
{
	unsigned int lich1, lich2, lich3, lich4;
	bool valid1 = CGolay24128::decode24128((unsigned char *)(fragment + 0U), lich1);
	bool valid2 = CGolay24128::decode24128((unsigned char *)(fragment + 3U), lich2);
	bool valid3 = CGolay24128::decode24128((unsigned char *)(fragment + 6U), lich3);
	bool valid4 = CGolay24128::decode24128((unsigned char *)(fragment + 9U), lich4);

	if (!valid1 || !valid2 || !valid3 || !valid4)
		return;

	unsigned char lich[M17_LICH_FRAGMENT_LENGTH_BYTES];
	CM17Utils::combineFragmentLICH(lich1, lich2, lich3, lich4, lich);

	unsigned int n = (lich4 >> 5) & 0x07U;
	m_running.setFragment(lich, n);

	bool valid = m_running.isValid();
	if (valid) {
		processLSF(m_running);
		m_running.reset();
	}
}

void CM17RX::addBleep()
{
	const unsigned int length = SOUNDCARD_SAMPLE_RATE / BLEEP_FREQ;
	const unsigned int total  = (SOUNDCARD_SAMPLE_RATE * BLEEP_LENGTH) / 1000U;

	float step = (2.0F * M_PI) / float(length);

	float audio[total];

	for (unsigned int i = 0U; i < total; i++)
		audio[i] = ::sinf(float(i) * step) * BLEEP_AMPL * m_volume;

	writeQueue(audio, total);
}

void CM17RX::addSilence(unsigned int n)
{
	const float SILENCE = 0.0F;

	for (unsigned int i = 0U; i < n; i++) {
		for (unsigned int j = 0U; j < SOUNDCARD_BLOCK_SIZE; j++)
			writeQueue(&SILENCE, 1U);
	}
}
