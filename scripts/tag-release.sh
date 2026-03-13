#!/usr/bin/env bash
set -e

VERSION="${1}"
if [ -z "$VERSION" ]; then
    echo "Usage: $0 <version>   e.g.  $0 v0.2.0"
    exit 1
fi

# Validate format
if ! echo "$VERSION" | grep -qE '^v[0-9]+\.[0-9]+\.[0-9]+$'; then
    echo "Error: version must be in the form v1.2.3"
    exit 1
fi

echo "Staging all changes..."
git add -A

# Only commit if there's something staged
if ! git diff --cached --quiet; then
    git commit -m "Release $VERSION"
fi

echo "Pushing commits..."
git push

echo "Tagging $VERSION..."
git tag "$VERSION"
git push origin "$VERSION"

echo ""
echo "Done. GitHub Actions will now build and publish the release."
echo "Watch it at: https://github.com/$(git remote get-url origin | sed 's/.*github.com[:/]\(.*\)\.git/\1/' | sed 's/.*github.com[:/]//')/actions"
