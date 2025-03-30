#!/bin/bash

# Check if a filename is provided
if [ -z "CMakeLists.txt" ]; then
    echo "CMakeLists.txt not found"
    exit 1
fi

# Extract the version string using sed
version=$(sed -n 's/set(PROJECT_VER "\([^"]*\)").*/\1/p' "CMakeLists.txt")

# Check if a version was found
if [ -n "$version" ]; then
    echo "Copying q-tune-$version.bin to the web installer"
    cp build/q-tune-$version.bin ../q-tune-installer/docs/artifacts/
    cp build/partition_table/partition-table.bin ../q-tune-installer/docs/artifacts/
    cp build/bootloader/bootloader.bin ../q-tune-installer/docs/artifacts/
else
    echo "Version string not found."
    exit 1
fi

filename="../q-tune-installer/docs/artifacts/manifest.json"

# Use jq to update the version fields
jq --arg version "$version" '
    .version = $version |
    .builds[].parts[].path |= sub("[0-9]+\\.[0-9]+\\.[0-9]+"; $version)
' "$filename" > tmp.json && mv tmp.json "$filename"

pushd ../q-tune-installer \
  && git add docs/artifacts/q-tune-$version.bin \
  && git commit -a -m "Update to v$version" \
  && git push origin \
  && popd

echo "Version updated to $version in $filename"
