FROM gcr.io/customer-mol/gce-kube-registry/mojolicious-image
MAINTAINER Joeri van Dooren

# Copy app
COPY app /app

# Carton install
RUN (cd /app && rm -fr local/ && carton install)

# Make it executable
RUN chmod a+rx /app/*.sh

ENTRYPOINT ["/app/run.sh"]

# Set labels used in Docker to describe the builder images
LABEL io.k8s.description="Mojolicious" \
      io.k8s.display-name="mojolicious perl" \
      io.openshift.expose-services="80:http" \
      io.openshift.tags="builder,mojolicious,perl"
