<?php

define('HISTORY_DIR', __DIR__ . '/history');
define('MAX_OBJECTS_PER_FILE', 50);

/**
 * Reads the nth JSON file (ordered by file number ascending).
 * n = 0 returns the oldest file, n = 1 the second oldest, etc.
 *
 * Returns:
 *   - array of objects on success
 *   - null if the requested file does not exist
 */
function historyGet(int $n): ?array
{
    if ($n < 0) {
        return null;
    }

    $files = historyListFiles();

    if (!isset($files[$n])) {
        return null;
    }

    $contents = @file_get_contents($files[$n]);
    if ($contents === false) {
        return null;
    }

    $data = json_decode($contents);

    // Ensure it is an array.
    if (!is_array($data)) {
        return [];
    }

    return $data;
}

/**
 * Appends an object to the newest history file.
 * If the newest file has MAX_OBJECTS_PER_FILE entries,
 * a new numbered file is created.
 *
 * If the newest entry:
 *   - has the same reason,
 *   - has the same author,
 *   - was created within the last 2 minutes,
 *   - has fewer than 5 sectors,
 *   - and the resulting sectors field would be <= 80 characters,
 * then the new sector is appended to its sectors field instead
 * of creating a new history entry.
 *
 * Returns true on success, false on failure.
 */
function historyAppend(object $object): bool
{
    if (!is_dir(HISTORY_DIR)) {
        if (!mkdir(HISTORY_DIR, 0777, true)) {
            return false;
        }
    }

    $files = historyListFiles();

    // No history files yet.
    if (empty($files)) {
        return historyWriteFile(1, [$object]);
    }

    $latestFile = end($files);
    $latestNumber = (int)pathinfo($latestFile, PATHINFO_FILENAME);

    $contents = @file_get_contents($latestFile);
    $data = json_decode($contents);

    if (!is_array($data)) {
        $data = [];
    }

    // Attempt to merge with the previous entry.
    if (!empty($data)) {
        $lastIndex = count($data) - 1;
        $last = $data[$lastIndex];

        if (
            isset($last->reason, $last->author, $last->time_n, $last->sectors) &&
            isset($object->reason, $object->author, $object->time_n, $object->sectors) &&
            $last->reason === $object->reason &&
            $last->author === $object->author &&
            ($object->time_n - $last->time_n) <= 120
        ) {
            $sectorList = explode(" ", $last->sectors);

            if (count($sectorList) < 32) {
                // Avoid duplicate sectors.
                if (!in_array($object->sectors, $sectorList, true)) {

                    $newSectors = $last->sectors . " " . $object->sectors;

                    // Only merge if the resulting string is not too long.
                    if (strlen($newSectors) <= 768) {
                        $last->sectors = $newSectors;
                        $data[$lastIndex] = $last;

                        return file_put_contents(
                            $latestFile,
                            json_encode($data, JSON_UNESCAPED_SLASHES)
                        ) !== false;
                    }
                } else {
                    // Duplicate sector; nothing to change.
                    return true;
                }
            }
        }
    }

    // Create a new file if this one is full.
    if (count($data) >= MAX_OBJECTS_PER_FILE) {
        return historyWriteFile($latestNumber + 1, [$object]);
    }

    // Otherwise append normally.
    $data[] = $object;

    return file_put_contents(
        $latestFile,
        json_encode($data, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES)
    ) !== false;
}
/**
 * Returns all valid history files sorted numerically.
 * Ignores files that are not of the form "<number>.json".
 */
function historyListFiles(): array
{
    if (!is_dir(HISTORY_DIR)) {
        return [];
    }

    $files = [];

    foreach (scandir(HISTORY_DIR) as $entry) {
        if (preg_match('/^([1-9][0-9]*)\.json$/', $entry, $matches)) {
            $files[(int)$matches[1]] = HISTORY_DIR . '/' . $entry;
        }
    }

    ksort($files, SORT_NUMERIC);

    return array_values($files);
}

/**
 * Writes a numbered history file.
 */
function historyWriteFile(int $number, array $data): bool
{
    return file_put_contents(
        HISTORY_DIR . '/' . $number . '.json',
        json_encode($data, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES)
    ) !== false;
}

/**
 * Returns the number of the most recent history file.
 *
 * Returns:
 *   - The file number (e.g. 7)
 *   - 0 if no valid history files exist
 */
function historyLatestNumber(): int
{
    $files = historyListFiles();

    if (empty($files)) {
        return 0;
    }

    $latestFile = end($files);

    return (int)pathinfo($latestFile, PATHINFO_FILENAME);
}