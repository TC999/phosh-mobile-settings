# Copyright (C) 2026 Phosh.mobi e.V.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Mock org.freedesktop.sysupdate1 on the session bus
#
# python3 -m dbusmock --template tests/mocks/sysupdate.py

import json

import ctypes
import dbus
import dbusmock

from types import MethodType

import gi

from gi.repository import GLib

BUS_NAME = "org.freedesktop.sysupdate1"
MAIN_OBJ = "/org/freedesktop/sysupdate1"
MAIN_IFACE = "org.freedesktop.sysupdate1.Manager"
TARGET_IFACE =  "org.freedesktop.sysupdate1.Target"
JOB_IFACE = "org.freedesktop.sysupdate1.Job"

SYSTEM_BUS = False

TARGET_PATH = "/org/freedesktop/sysupdate1/target/host"

DEFAULT_JOB_PROGRESS = 10
DEFAULT_JOB_TIMEOUT = 500

artifacts = [
    ("efi", "ad61c68da62c7337f17a9b0f9040a8690c0820e2fa10d9fbee68b03af62257fe"),
    ("qcow2", "63a93fca485cadb45c56c1753765c0e88c261745ebd6d8b408e51f6318628dae"),
    ("usr", "c04f7e9b458a9b31b10b910053b84a2eef04e88c4b8e2c2cec792a3c932759fe"),
    ("verity", "85a67fbe4b6650e2fa3fdbac7c451fb64e4fcebd88307336b1b4eaf7525988be"),
]

def ListVersions(self):
    return self.versions

def Describe(self, ver):
    if ver == self.versions[0]:
        available = False
        installed = True
    else:
        available = True
        installed = False

    ret = json.dumps({
        "version": ver,
        "newest": True,
        "available": available,
        "installed": installed,
        "partial": False,
        "pending": False,
        "obsolete": False,
        "protected": False,
        "incomplete": False,
        "changelogUrls": [],
        "contents": [
            {
                "type": "regular-file",
                "path": "/boot/EFI/Linux/BengalOS_%s.efi" % ver,
                "sha256": artifacts[0][1],
            },
            {
                "type": "regular-file",
                "path": "/usr/lib/bengalos/usr.raw.xz",
                "sha256": artifacts[2][1],
            },
            {
                "type": "regular-file",
                "path": "/usr/lib/bengalos/usr-verity.raw.xz",
                "sha256": artifacts[3][1],
            },
        ],
    })
    return ret


def remove_job(self, job_path, exitcode=0):
    # Emit JobRemoved
    job = dbusmock.get_object(job_path)
    if job.timeout_id:
        print(f"Removing progress source id: {job.timeout_id}")
        GLib.source_remove(job.timeout_id)
        job.timeout_id = 0

    num = int(job_path.rsplit('/', maxsplit=1)[-1])
    self.EmitSignal(MAIN_IFACE,
                    "JobRemoved",
                    "toi",
                    [
                        num,
                        dbus.ObjectPath(job_path),
                        exitcode,
                    ])
    self.RemoveObject(job_path)


def update_job(target):
    job = dbusmock.get_object(target.job_path)
    job.progress += DEFAULT_JOB_PROGRESS
    job.Set(JOB_IFACE, "Progress",  dbus.UInt32(min(100, job.progress)))

    if job.progress >= 100:
        main_obj = dbusmock.get_object(MAIN_OBJ)
        remove_job(main_obj, target.job_path)
        return False

    return True


def CancelJob(job):
    main_obj = dbusmock.get_object(MAIN_OBJ)
    remove_job(main_obj, job.path, 15)


def AcquireOrInstall(target, ver):
    target.job_num += 1
    target.job_path = f"/org/freedesktop/sysupdate1/job/{target.job_num}"
    ret = (ver, target.job_num, target.job_path)

    target.AddObject(
        target.job_path,
        JOB_IFACE,
        {"Progress":  dbus.UInt32(0)},
        [
            ("Cancel", "", "", "self.CancelJob()"),
        ],
    )
    job = dbusmock.get_object(target.job_path)
    job.CancelJob = MethodType(CancelJob, job)

    job.progress = 0
    job.timeout_id = GLib.timeout_add(DEFAULT_JOB_TIMEOUT, target.update_job)

    return ret


def load(mock, _params):

    mock.AddMethod(
        MAIN_IFACE,
        "ListTargets",
        "",
        "a(sso)",
        "ret = [('host', 'host', '/org/freedesktop/sysupdate1/target/host')]",
    )

    mock.AddObject(
        TARGET_PATH,
        TARGET_IFACE,
        {},
        [
            ("List", "t", "as", "ret = self.ListVersions()"),
            ("Describe", "st", "s", "ret = self.Describe(args[0])"),
            ("Acquire", "st", "sto", "ret = self.Acquire(args[0])"),
            ("Install", "st", "sto", "ret = self.Install(args[0])"),
        ],
    )

    target = dbusmock.get_object(TARGET_PATH)
    target.versions = ["0.0.20260601.1", "0.0.20260604.1"]
    target.job_num = 0
    target.job_path = None

    target.ListVersions = MethodType(ListVersions, target)
    target.Describe = MethodType(Describe, target)
    target.Install = MethodType(AcquireOrInstall, target)
    target.Acquire = MethodType(AcquireOrInstall, target)
    target.update_job = MethodType(update_job, target)

    print(f"{BUS_NAME} mock running")
