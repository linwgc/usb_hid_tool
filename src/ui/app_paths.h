#pragma once

#include <QString>

/** Writable root for Log/, Data/csv/, settings. Prefers directory of the .exe; falls back to AppLocalData if not writable. */
QString appToolDataRoot();

QString appLogDir();
QString appDataDir();
