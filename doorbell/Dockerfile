FROM gcr.io/customer-mol/gce-kube-registry/mojolicious-image
COPY app /app

# Carton install
RUN (cd /app && carton install)

# Make it executable
RUN chmod a+rx /app/*.sh
