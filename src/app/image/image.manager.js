(function() {
    'use strict';

    angular
        .module('webviewer')
        .factory('wvImageManager', wvImageManager);

    /* @ngInject */
    function wvImageManager($http, $q, $compile, $timeout, $rootScope, wvConfig, WvImage, wvCornerstoneImageAdapter) {
        var service = {
            get: get,
            getPixelObject: getPixelObject,
            createAnnotedImage: createAnnotedImage,
            /**
             * @public register a post processor
             */
            registerPostProcessor: registerPostProcessor
        };
    
        var postProcessorClasses = {};

        // @todo flush somehow
        var modelCache = {};

        // @todo flush somehow
        var pixelCache = {};


        return service;

        ////////////////
        
        // @input id: string <slice-id>[|<processor>...]
        //    <slice-id>: *:\d+ (_instance_:_slice_)
        //    <processor>: <string>[~<string>...] (_name_~_param1_~_param2_...)
        function get(id) {
            var _this = this;

            if (!modelCache.hasOwnProperty(id)) {
                // split between image id and postProcesses
                var splitted = id.split('|');
                id = splitted[0];
                var postProcessesStrings = splitted.splice(1);
                var postProcesses = postProcessesStrings.map(function (processString) {
                    // split processString between process name and its arguments
                    splitted = processString.split('~');
                    var processName = splitted[0];
                    var processArgs = splitted.splice(1);

                    if (!postProcessorClasses.hasOwnProperty(processName)) {
                        throw new Error('wv-image: unknown post processor');
                    }
                    
                    var postProcessObject = new (Function.prototype.bind.apply(postProcessorClasses[processName], [null].concat(processArgs)));
                    return postProcessObject;
                });

                // split between dicom instance id and frame index
                splitted = id.split(':');
                var instanceId = splitted[0];
                var frameIndex = splitted[1] || 0;
                
                // return results
                modelCache[id] = $http
                    .get(wvConfig.orthancApiURL + '/instances/'+instanceId+'/simplified-tags', {cache: true})
                    .then(function(response) {
                        var tags = response.data;
                        return new WvImage(_this, id, tags, postProcesses);
                    });
            };

            return modelCache[id];
        };

        function getPixelObject(id) {
            if (!pixelCache.hasOwnProperty(id)) {
                var compression = wvConfig.defaultCompression;
                var splittedId = id.split(':');
                var instanceId = splittedId[0];
                var frameIndex = splittedId[1];

                var USE_REST_API_0_2 = true;
                if (USE_REST_API_0_2) {
                    pixelCache[id] = $http
                        .get(wvConfig.orthancApiURL + '/nuks/' + instanceId + '/' + frameIndex + '/8bit/' + 'jpeg:'+compression +'/klv', {
                            responseType: 'arraybuffer'
                        })
                        .then(function(response) {
                            // retrieve json and image from binary
                            var klvReader = new KLVReader(response.data);
                            
                            var keys = {
                                // - Cornerstone related
                                Color: 0,
                                Height: 1,
                                Width: 2,
                                SizeInBytes: 3, // size in raw prior to compression

                                // Pixel size / aspect ratio
                                ColumnPixelSpacing: 4,
                                RowPixelSpacing: 5,

                                // LUT
                                MinPixelValue: 6,
                                MaxPixelValue: 7,
                                Slope: 8,
                                Intercept: 9,
                                WindowCenter: 10,
                                WindowWidth: 11,


                                // - WebViewer related
                                IsSigned: 12,
                                Stretched: 13, // set back 8bit to 16bit if true
                                Compression: 14,


                                // - Image binary
                                ImageBinary: 15
                            };

                            var json = {
                                color: klvReader.getUInt(keys.Color),
                                height: klvReader.getUInt(keys.Height),
                                width: klvReader.getUInt(keys.Width),
                                rows: klvReader.getUInt(keys.Height),
                                columns: klvReader.getUInt(keys.Width),
                                sizeInBytes: klvReader.getUInt(keys.SizeInBytes),

                                columnPixelSpacing: klvReader.getFloat(keys.ColumnPixelSpacing),
                                rowPixelSpacing: klvReader.getFloat(keys.RowPixelSpacing),

                                minPixelValue: klvReader.getInt(keys.MinPixelValue),
                                maxPixelValue: klvReader.getInt(keys.MaxPixelValue),
                                slope: klvReader.getFloat(keys.Slope),
                                intercept: klvReader.getFloat(keys.Intercept),
                                windowCenter: klvReader.getFloat(keys.WindowCenter),
                                windowWidth: klvReader.getFloat(keys.WindowWidth),

                                Orthanc: {
                                    IsSigned: klvReader.getUInt(keys.IsSigned),
                                    Compression: klvReader.getString(keys.Compression),
                                    Stretched: klvReader.getUInt(keys.Stretched),
                                    StretchLow: klvReader.getInt(keys.MinPixelValue),
                                    StretchHigh: klvReader.getInt(keys.MaxPixelValue),
                                    PixelData: klvReader.getBinary(keys.ImageBinary)
                                }
                            };

                            // adapt the image datas to cornerstone
                            return wvCornerstoneImageAdapter.process(id, json);
                        });
                }
                else {
                    pixelCache[id] = $http
                        .get(wvConfig.webviewerApiURL + '/instances/' +'jpeg'+compression+ '-' + instanceId + '_' + frameIndex, {cache: true})
                        .then(function(response) {
                            response.data.Orthanc.PixelData = _str2ab(window.atob(response.data.Orthanc.PixelData));
                            return wvCornerstoneImageAdapter.process(id, response.data);
                        });
                }
            }

            return pixelCache[id];
        }
        
        function registerPostProcessor(name, PostProcessor) {
            postProcessorClasses[name] = PostProcessor;
        }

        // @todo should be in @RootAggregate (ie. image-model)
        function createAnnotedImage(id, width, height) {
            // create a fake viewport containing the image to save it with the annotations

            // create a fake scope for the viewport
            var $scope = $rootScope.$new();
            $scope.size = {
                width: width + 'px',
                height: height + 'px'
            };
            $scope.imageId = id;

            var fakeViewport = $compile([
                '<wv-viewport id="FAKE-VIEWPORT-USED-IN-IMAGE-SERVICE"',
                    'wv-image-id="imageId"',
//                    'wv-viewport="$viewport"',
                    'wv-size="size"',
                    'wv-enable-overlay="false"',

                    'wv-angle-measure-viewport-tool="true"',
                    'wv-length-measure-viewport-tool="true"',
                    'wv-elliptical-roi-viewport-tool="true"',
                    'wv-zoom-viewport-tool="true"',
                    'wv-pan-viewport-tool="true"',
                    'wv-pixel-probe-viewport-tool="true"',
                    'wv-rectangle-roi-viewport-tool="true"',
//                    'wv-invert-contrast-viewport-tool="???"',
                '>',
                    // '<wv-overlay wv-tags="$image.tags" wv-viewport="$viewport" wv-show-timeline="false"></wv-overlay>',
                '</wv-viewport>'
            ].join('\n'))($scope);

            // happend the element to the body as it is required to define its size
            // make sure it's harmless
            var body = $('body');
            var _oldBodyOverflow = body.css('overflow');
            body.css('overflow', 'hidden');
            fakeViewport.css('position', 'absolute');
            fakeViewport.css('left', '-50000px');
            body.append(fakeViewport)

            function _destroyFakeViewport() {
                // revert body state
                body.css('overflow', _oldBodyOverflow);
                
                // remove element from body
                $('#FAKE-VIEWPORT-USED-IN-IMAGE-SERVICE').remove();

                // destroy useless scope
                $scope.$destroy();
            }


            // wait for the fakeViewport to have digested
            return $q(function(resolve, reject) {
                $timeout(function() {
                    var image = null;

                    // save the image to base64 data
                    var canvas = fakeViewport.find('canvas').get(0);
                    image = canvas.toDataURL();

                    _destroyFakeViewport();

                    resolve(image);
                });
            })
        }

    }

    // http://stackoverflow.com/a/11058858/881731
    function _str2ab(str) {
        var buf = new ArrayBuffer(str.length);
        var pixels = new Uint8Array(buf);
        for (var i = 0, strLen=str.length; i<strLen; i++) {
            pixels[i] = str.charCodeAt(i);
        }
        return pixels;
    }

})();

