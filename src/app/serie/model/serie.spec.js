/* jshint -W117, -W030 */
describe('serie', function() {
  describe('repository', function () {

    beforeEach(function() {
        bard.appModule('webviewer');
        
        bard.inject(this, '$controller', '$q', '$rootScope', '$timeout', '$httpBackend',
            'wvConfig', 'wvSerie', 'wvOrthancSerieAdapter', 'wvSerie');

        _.forEach(orthanc.raw, function(data, path) {
          $httpBackend
            .when('GET', '/' + path)
            .respond(data);
        });
    });

    bard.verifyNoOutstandingHttpRequests();

    it('should return promises', function() {
      // given
      var orthancSerie = orthanc.series.with2SingleFrameInstances;

      // when
      var serie = wvSerie.listFromOrthancSerieId(orthancSerie.ID);

      // then
      expect(serie.then).to.not.equal(undefined);

      $httpBackend.flush();
    });

    it('should work with orthanc multiframe instance', function(done) {
      // given a series containing 2 multiframe instances
      var orthancStudy = orthanc.studies.withMultiFrameInstances; // 25 wv series in that data sheet

      // when
      wvSerie
      .listFromOrthancStudyId(orthancStudy.ID)
      .then(function(wvSeries) {

        // then
        expect(wvSeries.length).to.equal(2);
        done();

      });

      $httpBackend.flush();
    });

    it('should work with orthanc mono instance', function(done) {
      // given
      var orthancSerie = orthanc.series.with2SingleFrameInstances;
      var orthancSortedInstances = orthanc.sortedInstances.with2SingleFrameInstances;

      // when
      wvSerie
      .listFromOrthancSerieId(orthancSerie.ID)
      .then(function(wvSerie) {

        // then
        var expectedResult = wvOrthancSerieAdapter.process(orthancSerie, orthancSortedInstances);
        expect(wvSerie.length).to.equal(expectedResult.length);
        expect(wvSerie.id).to.equal(expectedResult.id);
        expect(wvSerie.tags).to.deep.equal(expectedResult.tags);
        expect(wvSerie.imageIds).to.deep.equal(expectedResult.imageIds);
        done();

      });

      $httpBackend.flush();
    });

  });
});
