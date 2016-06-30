describe('backend', function() {

    this.timeout(30000); // Set a long timeout because image compression can takes longer than the default 2s timeout

    beforeEach(function() {
        bard.asyncModule('webviewer', function(wvConfigProvider) {
            wvConfigProvider.setApiURL(window.orthancUrl || '/');
        });
        bard.inject('wvSeriesManager', 'wvImageBinaryManager', 'WvImageQualities');

        seriesId = '7982dce8-d6a3ce66-d6fac396-d2427a98-61d94367:0';
        seriesId2 = '5910c9dd-4c2f8394-a9d63c4a-983e3837-7acded9b:0';
        imageId = '04389b99-731fd35c-a8ba10a0-a1d9cb32-d7dbd903:0';
    });

    var seriesId, seriesId2;
    var seriesTags = {
      "AcquisitionDate": "20121009",
      "AcquisitionDeviceProcessingCode": "0706",
      "AcquisitionDeviceProcessingDescription": "CHEVILLE droite",
      "AcquisitionNumber": "026",
      "AcquisitionTime": "104718.046",
      "AnnotationDisplayFormatID": "CR00",
      "BitsAllocated": "16",
      "BitsStored": "10",
      "BodyPartExamined": "LOW_EXM",
      "BorderDensity": "BLACK",
      "Columns": "1770",
      "ContentDate": "20130812",
      "ContentTime": "104729.859",
      "ContrastBolusAgent": "",
      "FilmOrientation": "LANDSCAPE",
      "GenericGroupLength": "74",
      "HighBit": "9",
      "ImageDisplayFormat": "STANDARD\\1,1",
      "ImageType": "DERIVED\\PRIMARY\\POST_PROCESSED\\\\\\\\\\\\100000",
      "ImagerPixelSpacing": "0.10\\0.10",
      "InstanceNumber": "1002",
      "InstitutionAddress": "The Hospital Address",
      "InstitutionName": "The Hospital",
      "Laterality": "",
      "LossyImageCompression": "00",
      "Manufacturer": "FUJIFILM Corporation",
      "Modality": "CR",
      "OperatorsName": "THE^OPERATOR",
      "PatientBirthDate": "19720405",
      "PatientID": "2345",
      "PatientIdentityRemoved": "YES",
      "PatientName": "ORTHO",
      "PatientOrientation": "",
      "PerformingPhysicianName": "THE^DOCTOR",
      "PhotometricInterpretation": "MONOCHROME1",
      "PhysiciansOfRecord": "THE^DOCTOR",
      "PixelData": null,
      "PixelRepresentation": "0",
      "PixelSpacing": "0.10\\0.10",
      "PlateID": "a48247658c",
      "PositionerType": "NONE",
      "ProtocolName": "The Protocol",
      "RETIRED_StudyComments": "",
      "RequestingPhysician": "THE^OTHER^DOCTOR",
      "RequestingService": "SYNAPSE DEFAULT",
      "RescaleIntercept": "0",
      "RescaleSlope": "1",
      "RescaleType": "us",
      "Rows": "2370",
      "SOPClassUID": "1.2.840.10008.5.1.4.1.1.1",
      "SOPInstanceUID": "1.2.276.0.7230010.3.1.4.2344313775.14992.1458058359.6813",
      "SamplesPerPixel": "1",
      "Sensitivity": "264",
      "SeriesDate": "20130812",
      "SeriesDescription": "CHEVILLE droite",
      "SeriesInstanceUID": "1.2.276.0.7230010.3.1.3.2344313775.14992.1458058359.6812",
      "SeriesNumber": "1002",
      "SeriesTime": "104718.000",
      "SpecificCharacterSet": "ISO_IR 100",
      "StudyDate": "20130812",
      "StudyDescription": "LOW_EXM",
      "StudyInstanceUID": "1.2.276.0.7230010.3.1.2.2344313775.14992.1458058359.6811",
      "StudyTime": "104251",
      "Trim": "NO",
      "ViewPosition": "",
      "WindowCenter": "511",
      "WindowWidth": "1023"
    };
    var seriesOrderedImageIds = [
      "f29eff5d-ab208b3c-fa1735d3-ebc4aa2f-284fa0cc:0",
      "2b22dad1-5b26015e-68493e3d-83a9854b-91a16d53:0"
    ];
    var seriesAvailableImageQualities = [
        'LOSSLESS', 'MEDIUM', 'LOW'
    ];

    describe('route /series/<instance>', function() {

        it('should provide standard tags', function() {
            // Retrieve a series
            return wvSeriesManager
                .get(seriesId)
                .then(function(series) {
                    // Validate series contains the tags
                    assert.deepEqual(series.tags, seriesTags);
                }, function(error) {
                    // Fail the test - series not found
                    assert.fail();
                });
        });

        it('should provide an ordered list of images', function() {
            // Retrieve a series
            return wvSeriesManager
                .get(seriesId2)
                .then(function(series) {
                    // Validate series contains the ordered images
                    assert.deepEqual(series.imageIds, seriesOrderedImageIds);
                }, function(error) {
                    // Fail the test - series not found
                    assert.fail();
                });
        });
        
        it('should provide available image qualities', function() {
            // Retrieve a series
            return wvSeriesManager
                .get(seriesId)
                .then(function(series) {
                    // Validate series contains the available image qualities
                    assert.deepEqual(series.availableQualities, seriesAvailableImageQualities);
                }, function(error) {
                    // Fail the test - series not found
                    assert.fail();
                });
        });

        it('should return a 404 error when the series doesn\'t exists', function() {
            // Retrieve an inexistant series
            return wvSeriesManager
                .get('my-inexistant-series:35')
                .then(function(series) {
                    // Fail the test - series shouldn't have been found
                    assert.fail();
                }, function(error) {
                    // Validate the returned code is 404
                    assert.equal(error.status, 404);
                });
        });

    });

    var imageId;
    var imageResolution = {
        // width, height
        highQuality: [1770, 2370],
        mediumQuality: [746, 1000],
        lowQuality: [112, 150]
    };

    describe('route /images/<instance>/high-quality', function() {

        it('should load an high quality version of the image', function() {
            // Retrieve image with LOSSLESS quality
            return wvImageBinaryManager
                .get(imageId, WvImageQualities.LOSSLESS)
                .then(function(pixelObject) {
                    // Succeed if image has been retrieved
                    assert.ok(true);

                    // Check the image has high quality
                    assert.equal(pixelObject.width, imageResolution.highQuality[0]);
                    assert.equal(pixelObject.height, imageResolution.highQuality[1]);
                }, function(error) {
                    // Fail on error - if image should have been retrieved
                    assert.fail();
                });
        });

        it('should fail on inexistant image', function() {
            // Retrieve inexistant image
            return wvImageBinaryManager
                .get('robocop:34', WvImageQualities.LOSSLESS)
                .then(function() {
                    // Fail if inexistant image request returns successful result
                    assert.fail();
                }, function(error) {
                    // Succeed on error - if image has not been retrieved
                    // Validate the returned code is 404
                    assert.equal(error.status, 404);
                });
        });

    });

    describe('route /images/<instance>/medium-quality', function() {

        it('should load an medium quality version of the image', function() {
            // Retrieve image with MEDIUM quality
            return wvImageBinaryManager
                .get(imageId, WvImageQualities.MEDIUM)
                .then(function(pixelObject) {
                    // Succeed if image has been retrieved
                    assert.ok(true);

                    // Check the image has medium quality
                    assert.equal(pixelObject.width, imageResolution.mediumQuality[0]);
                    assert.equal(pixelObject.height, imageResolution.mediumQuality[1]);

                }, function(error) {
                    // Fail on error - if image should have been retrieved
                    assert.fail();
                });
        });

        it('should fail on inexistant image', function() {
            // Retrieve inexistant image
            return wvImageBinaryManager
                .get('robocop:34', WvImageQualities.MEDIUM)
                .then(function() {
                    // Fail if inexistant image request returns successful result
                    assert.fail();
                }, function(error) {
                    // Succeed on error - if image has not been retrieved
                    // Validate the returned code is 404
                    assert.equal(error.status, 404);
                });
        });

    });

    describe('route /images/<instance>/low-quality', function() {

        it('should load an low quality version of the image', function() {
            // Retrieve image with LOW quality
            return wvImageBinaryManager
                .get(imageId, WvImageQualities.LOW)
                .then(function(pixelObject) {
                    // Succeed if image has been retrieved
                    assert.ok(true);
                    
                    // Check the image has low quality
                    assert.equal(pixelObject.width, imageResolution.lowQuality[0]);
                    assert.equal(pixelObject.height, imageResolution.lowQuality[1]);
                }, function(error) {
                    // Fail on error - if image should have been retrieved
                    assert.fail();
                });
        });

        it('should fail on inexistant image', function() {
            // Retrieve inexistant image
            return wvImageBinaryManager
                .get('robocop:34', WvImageQualities.LOW)
                .then(function() {
                    // Fail if inexistant image request returns successful result
                    assert.fail();
                }, function(error) {
                    // Succeed on error - if image has not been retrieved
                    // Validate the returned code is 404
                    assert.equal(error.status, 404);
                });
        });

    });

});
