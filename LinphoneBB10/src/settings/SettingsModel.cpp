/*
 * SettingsModel.cpp
 * Copyright (C) 2015  Belledonne Communications, Grenoble, France
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *  Created on: 24 août 2015
 *      Author: Sylvain Berfini
 */

#include "SettingsModel.h"

#include <bb/system/SystemProgressToast>
#include <bb/system/SystemUiProgressState>
#include <bb/system/SystemUiPosition>
#include <bb/system/InvokeManager.hpp>

using namespace bb::system;

SettingsModel::SettingsModel(QObject *parent)
    : QObject(parent),
      _manager(LinphoneManager::getInstance()),
      _accountSettingsModel(new AccountSettingsModel(this)),
      _isSrtpSupported(false),
      _isZrtpSupported(false),
      _isDtlsSupported(false)
{
    LinphoneCore *lc = _manager->getLc();
    if (lc) {
        _isSrtpSupported = linphone_core_media_encryption_supported(lc, LinphoneMediaEncryptionSRTP);
        _isZrtpSupported = linphone_core_media_encryption_supported(lc, LinphoneMediaEncryptionZRTP);
        _isDtlsSupported = linphone_core_media_encryption_supported(lc, LinphoneMediaEncryptionDTLS);
    }
}

void SettingsModel::setPayloadEnable(QString mime, int bitrate, bool enable) {
    LinphonePayloadType *payload = linphone_core_find_payload_type(_manager->getLc(), mime.toUtf8().constData(), bitrate, LINPHONE_FIND_PAYLOAD_IGNORE_CHANNELS);
    if (payload) {
        linphone_core_enable_payload_type(_manager->getLc(), payload, enable);
    }
}

bool SettingsModel::debugEnabled() const {
    LpConfig *lpc = linphone_core_get_config(_manager->getLc());
    return lp_config_get_int(lpc, "app", "debug", 0) == 1;
}

void SettingsModel::setDebugEnabled(const bool& enabled) {
    OrtpLogLevel logLevel = static_cast<OrtpLogLevel>(ORTP_LOGLEV_END);
    if (enabled) {
        logLevel = static_cast<OrtpLogLevel>(ORTP_MESSAGE | ORTP_WARNING | ORTP_FATAL | ORTP_ERROR);
    }
    linphone_core_set_log_level_mask(logLevel);

    LpConfig *lpc = linphone_core_get_config(_manager->getLc());
    lp_config_set_int(lpc, "app", "debug", enabled ? 1 : 0);
    lp_config_sync(lpc);
}

bool SettingsModel::logCollectionEnabled() const {
    return linphone_core_log_collection_enabled();
}

void SettingsModel::setLogCollectionEnabled(const bool& enabled) {
    linphone_core_enable_log_collection(enabled ? LinphoneLogCollectionEnabled : LinphoneLogCollectionDisabled);
    emit logsCollectionSettingUpdated();

    LpConfig *lpc = linphone_core_get_config(_manager->getLc());
    lp_config_set_int(lpc, "app", "log_collection", enabled ? 1 : 0);
}

static void log_collection_upload_progress(LinphoneCore *lc, size_t offset, size_t total) {
    LinphoneCoreVTable *vtable = linphone_core_get_current_vtable(lc);
    SystemProgressToast* log_upload_toast = (SystemProgressToast *)linphone_core_v_table_get_user_data(vtable);
    int progress = (int)offset * 100 / total;
    log_upload_toast->setProgress(progress);
    log_upload_toast->setStatusMessage(QString("%0%").arg(progress));
    log_upload_toast->update();
}

static void log_collection_upload_state(LinphoneCore *lc, LinphoneCoreLogCollectionUploadState state, const char *info) {
    LinphoneCoreVTable *vtable = linphone_core_get_current_vtable(lc);
    SystemProgressToast* log_upload_toast = (SystemProgressToast *)linphone_core_v_table_get_user_data(vtable);

    if (state == LinphoneCoreLogCollectionUploadStateDelivered || state == LinphoneCoreLogCollectionUploadStateNotDelivered) {
        linphone_core_remove_listener(lc, vtable);
        log_upload_toast->cancel();

        if (state == LinphoneCoreLogCollectionUploadStateDelivered) {
            QString recipient = "linphone-blackberry@belledonne-communications.com";
            QString subject = QObject::tr("Linphone Blackberry 10 logs");
            QString body_start = QObject::tr("Please describe your issue here or this mail will be ignored: ");
            QString appVersion = LinphoneManager::getInstance()->appVersion();
            QString coreVersion = LinphoneManager::getInstance()->coreVersion();
            QString blackberryVersion = LinphoneManager::getInstance()->blackberryVersion();
            QString body = QString("%0\r\n\r\n%1\r\n%2\r\n%3\r\n\r\n%4").arg(body_start, appVersion, coreVersion, blackberryVersion, info);
            InvokeManager invokeManager;
            InvokeRequest invokeRequest;

            invokeRequest.setTarget("sys.pim.uib.email.hybridcomposer");
            invokeRequest.setAction("bb.action.OPEN, bb.action.SENDEMAIL");
            invokeRequest.setUri(QString("mailto:%0?subject=%1&body=%2").arg(recipient, subject, body));

            invokeManager.invoke(invokeRequest);
        }
    }
}

void SettingsModel::uploadLogs() {
    LinphoneCore *lc = _manager->getLc();

    LinphoneCoreVTable *vtable = (LinphoneCoreVTable*) malloc(sizeof(LinphoneCoreVTable));
    memset (vtable, 0, sizeof(LinphoneCoreVTable));
    vtable->log_collection_upload_progress_indication = log_collection_upload_progress;
    vtable->log_collection_upload_state_changed = log_collection_upload_state;

    SystemProgressToast* log_upload_toast;
    log_upload_toast = new SystemProgressToast();

    log_upload_toast->setBody(tr("Logs upload"));
    log_upload_toast->setProgress(0);
    log_upload_toast->setStatusMessage(QString("0%"));
    log_upload_toast->setState(SystemUiProgressState::Active);
    log_upload_toast->setPosition(SystemUiPosition::MiddleCenter);

    linphone_core_v_table_set_user_data(vtable, log_upload_toast);
    linphone_core_add_listener(lc, vtable);
    linphone_core_upload_log_collection(lc);
    log_upload_toast->show();
}

void SettingsModel::resetLogs() {
    linphone_core_reset_log_collection();
}

bool SettingsModel::adaptiveRateControl() const {
    return linphone_core_adaptive_rate_control_enabled(_manager->getLc());
}

void SettingsModel::setAdaptiveRateControl(const bool& enabled) {
    linphone_core_enable_adaptive_rate_control(_manager->getLc(), enabled);
}

QVariantMap SettingsModel::audioCodecs() const {
    QVariantMap codecs;

    const MSList *payloads = linphone_core_get_audio_codecs(_manager->getLc());
    while (payloads) {
        PayloadType *payload = (PayloadType *) payloads->data;
        QString codecName = QString("%1 (%2 Hz)").arg(payload->mime_type, QString::number(payload_type_get_rate(payload)));

        QVariantList infos;
        infos << linphone_core_payload_type_enabled(_manager->getLc(), payload) << payload->mime_type << payload_type_get_rate(payload);
        codecs[codecName] = infos;

        payloads = ms_list_next(payloads);
    }

    return codecs;
}

bool SettingsModel::videoSupported() const {
    return linphone_core_video_supported(_manager->getLc());
}

bool SettingsModel::videoEnabled() const {
    return linphone_core_video_enabled(_manager->getLc());
}

void SettingsModel::setVideoEnabled(const bool& enabled) {
    linphone_core_enable_video(_manager->getLc(), enabled, enabled);
    emit settingsUpdated();
}

bool SettingsModel::previewVisible() const {
    return linphone_core_self_view_enabled(_manager->getLc());
}

void SettingsModel::setPreviewVisible(const bool& visible) {
    linphone_core_enable_self_view(_manager->getLc(), visible);
}

bool SettingsModel::outgoingVideoCalls() const {
    const LinphoneVideoPolicy *policy = linphone_core_get_video_policy(_manager->getLc());
    if (!policy) {
        return false;
    }
    return policy->automatically_initiate;
}

void SettingsModel::setOutgoingVideoCalls(const bool& enabled) {
    LinphoneVideoPolicy policy;
    policy.automatically_accept = incomingVideoCalls();
    policy.automatically_initiate = enabled;
    linphone_core_set_video_policy(_manager->getLc(), &policy);
}

bool SettingsModel::incomingVideoCalls() const {
    const LinphoneVideoPolicy *policy = linphone_core_get_video_policy(_manager->getLc());
    if (!policy) {
        return false;
    }
    return policy->automatically_accept;
}

void SettingsModel::setIncomingVideoCalls(const bool& enabled) {
    LinphoneVideoPolicy policy;
    policy.automatically_accept = enabled;
    policy.automatically_initiate = outgoingVideoCalls();
    linphone_core_set_video_policy(_manager->getLc(), &policy);
}

int SettingsModel::preferredVideoSizeIndex() const {
    const char *videoSize = linphone_core_get_preferred_video_size_name(_manager->getLc());

    if (strcmp(videoSize, "vga") == 0) {
        return 0;
    } else if (strcmp(videoSize, "qvga") == 0) {
        return 1;
    }/* else if (strcmp(videoSize, "cif") == 0) {
        return 2;
    } else if (strcmp(videoSize, "qvga") == 0) {
        return 3;
    } else if (strcmp(videoSize, "qcif") == 0) {
        return 4;
    }*/
    return -1;
}

void SettingsModel::setPreferredVideoSize(const QString& videoSize) {
    linphone_core_set_preferred_video_size_by_name(_manager->getLc(), videoSize.toLower().toUtf8().constData());
}

QVariantMap SettingsModel::videoCodecs() const {
    QVariantMap codecs;

    const MSList *payloads = linphone_core_get_video_codecs(_manager->getLc());
    while (payloads) {
        PayloadType *payload = (PayloadType *) payloads->data;
        QString codecName = payload->mime_type;

        QVariantList infos;
        infos << linphone_core_payload_type_enabled(_manager->getLc(), payload) << payload->mime_type << payload_type_get_rate(payload);
        codecs[codecName] = infos;

        payloads = ms_list_next(payloads);
    }

    return codecs;
}

int SettingsModel::mediaEncryption() const {
    return (int)linphone_core_get_media_encryption(_manager->getLc());
}

void SettingsModel::setMediaEncryption(const int& mediaEncryption) {
    linphone_core_set_media_encryption(_manager->getLc(), (LinphoneMediaEncryption)mediaEncryption);
}

bool SettingsModel::mediaEncryptionMandatory() const {
    return linphone_core_is_media_encryption_mandatory(_manager->getLc());
}

void SettingsModel::setMediaEncryptionMandatory(const bool& enabled) {
    linphone_core_set_media_encryption_mandatory(_manager->getLc(), enabled);
}

QString SettingsModel::stunServer() const {
    return linphone_core_get_stun_server(_manager->getLc());
}

void SettingsModel::setStunServer(const QString& stunServer) {
    linphone_core_set_stun_server(_manager->getLc(), stunServer.toUtf8().constData());
    emit settingsUpdated();
}

bool SettingsModel::iceEnabled() const {
    return linphone_core_get_firewall_policy(_manager->getLc()) == LinphonePolicyUseIce;
}

void SettingsModel::setIceEnabled(const bool& enabled) {
    LinphoneFirewallPolicy policy = LinphonePolicyNoFirewall;
    if (enabled) {
        policy = LinphonePolicyUseIce;
    } else {
        if (stunServer().isEmpty()) {
            policy = LinphonePolicyNoFirewall;
        } else {
            policy = LinphonePolicyUseStun;
        }
    }
    linphone_core_set_firewall_policy(_manager->getLc(), policy);
}

bool SettingsModel::randomPorts() const {
    LCSipTransports transports;
    linphone_core_get_sip_transports(_manager->getLc(), &transports);
    return transports.udp_port == LC_SIP_TRANSPORT_RANDOM && transports.tcp_port == LC_SIP_TRANSPORT_RANDOM && transports.tls_port == LC_SIP_TRANSPORT_RANDOM;
}

void SettingsModel::setRandomPorts(const bool& enabled) {
    LCSipTransports transports;
    linphone_core_get_sip_transports(_manager->getLc(), &transports);
    if (enabled) {
        transports.udp_port = transports.tcp_port = transports.tls_port = LC_SIP_TRANSPORT_RANDOM;
    } else {
        transports.udp_port = transports.tcp_port = 5060;
        transports.tls_port = 5061;
    }
    linphone_core_set_sip_transports(_manager->getLc(), &transports);
}
