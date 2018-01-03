set -e

mkdir -p build/docker/
cp doc/calld-example.cfg build/clang.debug/calld build/docker/
cp Builds/Docker/Dockerfile-testnet build/docker/Dockerfile
mv build/docker/calld-example.cfg build/docker/calld.cfg
strip build/docker/calld
docker build -t call/calld:$CIRCLE_SHA1 build/docker/
docker tag call/calld:$CIRCLE_SHA1 call/calld:latest

if [ -z "$CIRCLE_PR_NUMBER" ]; then
  docker tag call/calld:$CIRCLE_SHA1 call/calld:$CIRCLE_BRANCH
fi
