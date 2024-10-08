var Module = {
    canvas: (() => {
        var canvas = document.getElementById('canvas');
        var webglContextLostHandler = (e) => {
            alert('WebGL context lost. You will need to reload the page.');
            e.preventDefault();
        };

        // As a default initial behavior, pop up an alert when webgl context is lost. To make your
        // application robust, you may want to override this behavior before shipping!
        // See http://www.khronos.org/registry/webgl/specs/latest/1.0/#5.15.2
        canvas.addEventListener('webglcontextlost', webglContextLostHandler, false);

        return canvas;
    })(),
};
