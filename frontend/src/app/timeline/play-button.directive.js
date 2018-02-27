/**
 * @ngdoc directive
 * @name webviewer.directive:wvPlayButton
 * 
 * @param {osimis.Series} wvSeries The model of the series, as provided by the
 *                                 `wvSeriesId` directive.
 *                                 
 * @param {boolean} [wvReadonly=false] Deactivate the directive's inputs.
 * 
 * @scope
 * @restrict Element
 * 
 * @description
 * The `wvPlayButton` directive displays a play control and a configuration panel.
 * Framerate is controlled via the configuration panel.
 *
 * This directive is used by the `wvTimelineControls` directive.
 **/
 (function() {
    'use strict';

    angular
        .module('webviewer')
        .directive('wvPlayButton', wvPlayButton);

    /* @ngInject */
    function wvPlayButton() {
        var directive = {
            bindToController: true,
            controller: Controller,
            controllerAs: 'vm',
            link: link,
            restrict: 'E',
            scope: {
                series: '=wvSeries',
                readonly: '=?wvReadonly'
            },
            templateUrl: 'app/timeline/play-button.directive.html'
        };
        return directive;

        function link(scope, element, attrs) {
        }
    }

    /* @ngInject */
    function Controller(wvSeriesPlayer) {
        // Set default values
        this.readonly = (typeof this.readonly === 'undefined') ? false : this.readonly;
        this.wvSeriesPlayer = wvSeriesPlayer;
    }

    Controller.prototype.play = function() {
        this.wvSeriesPlayer.play();
    };

    Controller.prototype.pause = function() {
        this.wvSeriesPlayer.pause();
    };

})();